/*
 * Cbench - A C benchmarking suite for Linux benchmarking.
 * Copyright (C) 2013  Markus Pargmann <mpargmann@allfex.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <cbench/system.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include <klib/printk.h>

#include <cbench/sha256.h>
#include <cbench/util.h>

static void system_cpu_calc_ids(struct system *sys)
{
	sha256_context ctx;
	int i;

	sha256_starts(&ctx);

	for (i = 0; i != sys->hw.nr_cpus; ++i) {
		struct system_cpu *cpu = &sys->hw.cpus[i];

		sha256_add(&ctx, &cpu->online);
		sha256_add(&ctx, &cpu->processor);
		sha256_add(&ctx, &cpu->core_id);
		sha256_add(&ctx, &cpu->physical_id);
		sha256_add(&ctx, &cpu->apicid);
		sha256_add(&ctx, &cpu->apicid_initial);
		sha256_add(&ctx, &cpu->cores);
		sha256_add(&ctx, &cpu->siblings);
		sha256_add(&ctx, &cpu->cpuid_level);
		sha256_add(&ctx, &cpu->cache_size);
		sha256_add(&ctx, &cpu->clflush_size);
		sha256_add(&ctx, &cpu->cache_alignment);
		sha256_add(&ctx, &cpu->family);
		sha256_add(&ctx, &cpu->model);
		sha256_add(&ctx, &cpu->stepping);
		sha256_add(&ctx, &cpu->fpu);
		sha256_add(&ctx, &cpu->fpu_exception);
		sha256_add(&ctx, &cpu->wp);
		sha256_add_str(&ctx, cpu->vendor_id);
		sha256_add_str(&ctx, cpu->model_name);
		sha256_add_str(&ctx, cpu->microcode);
		sha256_add_str(&ctx, cpu->address_sizes_str);
		sha256_add_str(&ctx, cpu->power_management);
		sha256_add_str(&ctx, cpu->flags);

		sha256_add(&ctx, &cpu->cpufreq);
		if (cpu->cpufreq) {
			if (!strcmp("userspace", cpu->governor))
				sha256_add(&ctx, &cpu->cur_freq);

			sha256_add(&ctx, &cpu->min_freq);
			sha256_add(&ctx, &cpu->max_freq);
			sha256_add_str(&ctx, cpu->avail_freq);
		}
	}

	sha256_finish_str(&ctx, sys->hw.cpus_sha256);
}

static void system_calc_ids(struct system *sys)
{
	sha256_context ctx;

	system_cpu_calc_ids(sys);

	sha256_starts(&ctx);
	sha256_add(&ctx, &sys->hw.mem);
	sha256_add_str(&ctx, sys->hw.cpus_sha256);
	sha256_add_str(&ctx, sys->sw.gcc);
	sha256_add_str(&ctx, sys->sw.libc);
	sha256_add_str(&ctx, sys->sw.libpthread);

	sha256_add_str(&ctx, sys->machine);
	sha256_add_str(&ctx, sys->kernel_release);

	sha256_finish_str(&ctx, sys->sha256);
}

static void system_cpu_cpufreq_free(struct system_cpu *cpu)
{
	free(cpu->governor);
	free(cpu->avail_freq);
}

static int system_cpu_cpufreq_init(struct system_cpu *cpu)
{
	char path[128];
	char *tmp_file;

	sprintf(path, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor",
			cpu->processor);

	tmp_file = file_read_complete(path);
	if (!tmp_file)
		goto error;
	str_strip(tmp_file);
	cpu->governor = tmp_file;

	sprintf(path, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_available_frequencies",
			cpu->processor);
	tmp_file = file_read_complete(path);
	if (!tmp_file)
		goto error_avail_freq;
	str_strip(tmp_file);
	cpu->avail_freq = tmp_file;

	sprintf(path, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq",
			cpu->processor);
	tmp_file = file_read_complete(path);
	if (!tmp_file)
		goto error_max_freq;
	cpu->max_freq = atoi(tmp_file);
	free(tmp_file);

	sprintf(path, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq",
			cpu->processor);
	tmp_file = file_read_complete(path);
	if (!tmp_file)
		goto error_max_freq;
	cpu->min_freq = atoi(tmp_file);
	free(tmp_file);

	sprintf(path, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq",
			cpu->processor);
	tmp_file = file_read_complete(path);
	if (!tmp_file)
		goto error_max_freq;
	cpu->cur_freq = atoi(tmp_file);
	free(tmp_file);

	return 0;

error_max_freq:
	free(cpu->avail_freq);
error_avail_freq:
	free(cpu->governor);
error:
	printk(KERN_INFO "No cpufreq found for processor %d\n", cpu->processor);
	return -1;
}

static void system_cpus_free(struct system *sys)
{
	int nr_cpus = sys->hw.nr_cpus;
	struct system_cpu *cpus = sys->hw.cpus;
	int i;

	for (i = 0; i != nr_cpus; ++i) {
		struct system_cpu *cpu = &cpus[i];
		if (cpu->vendor_id)
			free(cpu->vendor_id);
		if (cpu->model_name)
			free(cpu->model_name);
		if (cpu->microcode)
			free(cpu->microcode);
		if (cpu->address_sizes_str)
			free(cpu->address_sizes_str);
		if (cpu->power_management)
			free(cpu->power_management);
		if (cpu->cpufreq)
			system_cpu_cpufreq_free(cpu);
	}
	free(sys->hw.cpus);
}

static int system_cpus_init(struct system *sys)
{
	struct system_cpu *cpus;
	struct system_cpu *cpu;
	int nr_cpus = get_nprocs();
	FILE *f;
	char *buf = NULL;
	int proc;
	int est_phys_procs = 1;
	int nr_phys_procs_on = 0;
	int nr_cores_on = 0;
	int nr_cpus_on = 0;
	int *present_cores;
	int i;
	int ret;
	size_t size;

	sys->hw.nr_cpus = nr_cpus;
	cpus = cpu = malloc(sizeof(*cpus) * nr_cpus);
	if (!cpus) {
		printk(KERN_ERR "Out of memory\n");
		return -1;
	}

	memset(cpus, 0, sizeof(*cpus) * nr_cpus);
	sys->hw.cpus = cpus;


	f = fopen("/proc/cpuinfo", "r");
	if (!f) {
		printk(KERN_ERR "Failed to open /proc/cpuinfo\n");
		free(cpus);
		return -1;
	}

	while (getline(&buf, &size, f) != -1) {
		char *sep = strchr(buf, ':');
		char *v_start;
		if (!sep)
			continue;

		*sep = '\0';
		++sep;
		v_start = sep;
		if (*v_start)
			++v_start;

		if (*v_start)
			v_start[strlen(v_start) - 1] = '\0';

		if (!strcmpb("processor", buf)) {
			proc = atoi(sep);
			cpu = &cpus[proc];
			cpu->processor = proc;
			cpu->online = 1;
			++nr_cpus_on;
		} else if (!strcmpb("vendor_id", buf)) {
			cpu->vendor_id = malloc(strlen(v_start) + 1);
			if (!cpu->vendor_id)
				goto error_alloc;
			strcpy(cpu->vendor_id, v_start);
		} else if (!strcmpb("cpu family", buf)) {
			cpu->family = atoi(v_start);
		} else if (!strcmpb("model name", buf)) {
			cpu->model_name = malloc(strlen(v_start) + 1);
			if (!cpu->model_name)
				goto error_alloc;
			strcpy(cpu->model_name, v_start);
		} else if (!strcmpb("model", buf)) {
			cpu->model = atoi(v_start);
		} else if (!strcmpb("stepping", buf)) {
			cpu->stepping = atoi(v_start);
		} else if (!strcmpb("microcode", buf)) {
			cpu->microcode = malloc(strlen(v_start) + 1);
			if (!cpu->microcode)
				goto error_alloc;
			strcpy(cpu->microcode, v_start);
		} else if (!strcmpb("cpu MHz", buf)) {
			cpu->mhz = atof(v_start);
		} else if (!strcmpb("cache size", buf)) {
			cpu->cache_size = atoi(v_start);
		} else if (!strcmpb("physical id", buf)) {
			cpu->physical_id = atoi(v_start);
			if (cpu->physical_id >= est_phys_procs)
				est_phys_procs = cpu->physical_id + 1;
		} else if (!strcmpb("siblings", buf)) {
			cpu->siblings = atoi(v_start);
		} else if (!strcmpb("core id", buf)) {
			cpu->core_id = atoi(v_start);
		} else if (!strcmpb("cpu cores", buf)) {
			cpu->cores = atoi(v_start);
		} else if (!strcmpb("apicid", buf)) {
			cpu->apicid = atoi(v_start);
		} else if (!strcmpb("initial apicid", buf)) {
			cpu->apicid_initial = atoi(v_start);
		} else if (!strcmpb("fpu_exception", buf)) {
			if (!strcmpb("yes", v_start))
				cpu->fpu = 1;
			else
				cpu->fpu = 0;
		} else if (!strcmpb("fpu", buf)) {
			if (!strcmpb("yes", v_start))
				cpu->fpu = 1;
			else
				cpu->fpu = 0;
		} else if (!strcmpb("cpuid level", buf)) {
			cpu->cpuid_level = atoi(v_start);
		} else if (!strcmpb("wp", buf)) {
			if (!strcmpb("yes", v_start))
				cpu->wp = 1;
			else
				cpu->wp = 0;
		} else if (!strcmpb("flags", buf)) {
			cpu->flags = malloc(strlen(v_start) + 1);
			if (!cpu->flags)
				goto error_alloc;
			strcpy(cpu->flags, v_start);
		} else if (!strcmpb("bogomips", buf)) {
			cpu->bogomips = atof(v_start);
		} else if (!strcmpb("clflush size", buf)) {
			cpu->clflush_size = atoi(v_start);
		} else if (!strcmpb("cache_alignment", buf)) {
			cpu->cache_alignment = atoi(v_start);
		} else if (!strcmpb("address sizes", buf)) {
			cpu->address_sizes_str = malloc(strlen(v_start) + 1);
			if (!cpu->address_sizes_str)
				goto error_alloc;
			strcpy(cpu->address_sizes_str, v_start);
		} else if (!strcmpb("power management", buf)) {
			cpu->power_management = malloc(strlen(v_start) + 1);
			if (!cpu->power_management)
				goto error_alloc;
			strcpy(cpu->power_management, v_start);
		} else {
			printk(KERN_ERR "The cpuinfo parser is out of date. There is a field unknown to this parser: %s\n",
					buf);
		}
	}
	if (buf)
		free(buf);

	ret = 0;
	for (i = 0; i != nr_cpus; ++i) {
		cpu = &cpus[i];
		ret = system_cpu_cpufreq_init(cpu);
		if (ret)
			cpu->cpufreq = 0;
		else
			cpu->cpufreq = 1;
	}

	present_cores = malloc(sizeof(*present_cores) * est_phys_procs * nr_cpus);
	if (!present_cores)
		goto error_alloc;
	memset(present_cores, 0, sizeof(*present_cores) * est_phys_procs * nr_cpus);

	for (i = 0; i != nr_cpus; ++i) {
		cpu = &cpus[i];
		if (!cpu->online)
			continue;
		present_cores[cpu->physical_id * nr_cpus + cpu->core_id] = 1;
	}

	for (i = 0; i != est_phys_procs; ++i) {
		int j;
		int first = 1;
		for (j = 0; j != nr_cpus; ++j) {
			if (!present_cores[i * nr_cpus + j])
				continue;
			if (first) {
				++nr_phys_procs_on;
				first = 0;
			}
			++nr_cores_on;
		}
	}
	sys->hw.nr_physical_procs_on = nr_phys_procs_on;
	sys->hw.nr_cores_on = nr_cores_on;
	sys->hw.nr_cpus_on = nr_cpus_on;

	free(present_cores);

	return 0;

error_alloc:
	printk(KERN_ERR "Out of memory\n");
	system_cpus_free(sys);
	return -1;
}


int system_info_init(struct system *sys)
{
	size_t s = 0;
	int ret;

	s = confstr(_CS_GNU_LIBC_VERSION, NULL, (size_t) 0);
	sys->sw.libc = malloc(s);
	if (!sys->sw.libc) {
		printk(KERN_ERR "Out of memory\n");
		return -1;
	}
	s = confstr(_CS_GNU_LIBC_VERSION, sys->sw.libc, s);
	if (!s) {
		printk(KERN_ERR "Failed acquiring libc info\n");
		goto error_libc_acq;
	}

	s = confstr(_CS_GNU_LIBPTHREAD_VERSION, NULL, (size_t) 0);
	sys->sw.libpthread = malloc(s);
	if (!sys->sw.libpthread) {
		printk(KERN_ERR "Out of memory\n");
		goto error_libc_acq;
	}
	s = confstr(_CS_GNU_LIBPTHREAD_VERSION, sys->sw.libpthread, s);
	if (!s) {
		printk(KERN_ERR "Failed acquiring libpthread info\n");
		goto error_libpthread_acq;
	}

	ret = uname(&sys->raw.uname);
	if (ret) {
		printk(KERN_ERR "Failed getting uname infos\n");
		goto error_libpthread_acq;
	}

	ret = sysinfo(&sys->raw.sysinfo);
	if (ret) {
		printk(KERN_ERR "Failed getting sysinfo\n");
		goto error_libpthread_acq;
	}

#define GCC_VER_TO_STRING(major, minor, patch) #major "." #minor "." #patch
	sys->sw.gcc = GCC_VER_TO_STRING(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#undef GCC_VER_TO_STRING

	sys->machine = sys->raw.uname.machine;
	sys->kernel_release = sys->raw.uname.release;

	sys->hw.mem.mem_total = sys->raw.sysinfo.totalram * sys->raw.sysinfo.mem_unit;
	sys->hw.mem.mem_totalhigh = sys->raw.sysinfo.totalhigh * sys->raw.sysinfo.mem_unit;
	sys->hw.mem.swap_total = sys->raw.sysinfo.totalswap * sys->raw.sysinfo.mem_unit;
	sys->hw.mem.mem_unit = sys->raw.sysinfo.mem_unit;

	ret = system_cpus_init(sys);
	if (ret)
		goto error_libpthread_acq;

	system_calc_ids(sys);

	return 0;

error_libpthread_acq:
	free(sys->sw.libpthread);

error_libc_acq:
	free(sys->sw.libc);
	return -1;
}

void system_info_free(struct system *sys)
{
	system_cpus_free(sys);
	free(sys->sw.libpthread);
	free(sys->sw.libc);
}

int system_info_comma_hdr(struct system *sys, char **buf, size_t *buf_len)
{
	int ret;

	ret = mem_grow((void**)buf, buf_len, 1024);
	if (ret)
		return -1;

	strcpy(*buf,
		"machine,kernel,"
		"nr_cpus,nr_cpus_on,nr_cores_on,"
		"physical_procs_on,mem_total,mem_totalhigh,"
		"mem_unit,swap_total,cpus_id");
	return 0;
}

int system_info_comma_str(struct system *sys, char **buf, size_t *buf_len)
{
	int ret;

	ret = mem_grow((void**)buf, buf_len, 1024);
	if (ret)
		return ret;
	sprintf(*buf,
			"\"%s\",\"%s\","
			"%d,%d,%d,"
			"%d,%lu,%lu,"
			"%lu,%lu,\"%s\"",
			sys->machine, sys->kernel_release,
			sys->hw.nr_cpus, sys->hw.nr_cpus_on, sys->hw.nr_cores_on,
			sys->hw.nr_physical_procs_on, sys->hw.mem.mem_total, sys->hw.mem.mem_totalhigh,
			sys->hw.mem.mem_unit, sys->hw.mem.swap_total, sys->hw.cpus_sha256);
	return 0;
}

int system_cpu_comma_hdr(struct system_cpu *cpu, char **buf, size_t *buf_len)
{
	int ret;

	ret = mem_grow((void**)buf, buf_len, 1024);
	if (ret)
		return -1;

	strcpy(*buf,
			"processor,online,core_id,"
			"physical_id,apicid,apicid_initial,"
			"cores,siblings,cpuid_level,"
			"cache_size,clflush_size,cache_alignment,"
			"mhz,bogomips,family,"
			"model,stepping,fpu,"
			"fpu_exception,wp,vendor_id,"
			"model_name,microcode,address_sizes,"
			"power_management,flags,governor,"
			"avail_freq,min_freq,max_freq,"
			"cur_freq");
	return 0;
}

int system_cpu_comma_str(struct system_cpu *cpu, char **buf, size_t *buf_len)
{
	int ret;

	ret = mem_grow((void**)buf, buf_len, 1024);
	if (ret)
		return -1;

	sprintf(*buf,
			"%d,%d,%d,"
			"%d,%d,%d,"
			"%d,%d,%d,"
			"%d,%d,%d,"
			"%f,%f,%d,"
			"%d,%d,%d,"
			"%d,%d,\"%s\","
			"\"%s\",\"%s\",\"%s\","
			"\"%s\",\"%s\",\"%s\","
			"\"%s\",%d,%d,"
			"%d",
			cpu->processor, cpu->online, cpu->core_id,
			cpu->physical_id, cpu->apicid, cpu->apicid_initial,
			cpu->cores, cpu->siblings, cpu->cpuid_level,
			cpu->cache_size, cpu->clflush_size, cpu->cache_alignment,
			cpu->mhz, cpu->bogomips, cpu->family,
			cpu->model, cpu->stepping, cpu->fpu,
			cpu->fpu_exception, cpu->wp, cpu->vendor_id,
			cpu->model_name, cpu->microcode, cpu->address_sizes_str,
			cpu->power_management, cpu->flags, (cpu->cpufreq ? cpu->governor : "none"),
			(cpu->cpufreq ? cpu->avail_freq : ""), cpu->min_freq, cpu->max_freq,
			cpu->cur_freq);
	return 0;
}

