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

#include <cbench/data.h>
#include <cbench/sha256.h>
#include <cbench/util.h>

static void system_cpu_calc_ids(struct system *sys)
{
	sha256_context ctx_all;
	sha256_context ctx;
	sha256_context type;
	int i;

	sha256_starts(&ctx_all);

	for (i = 0; i != sys->hw.nr_cpus; ++i) {
		struct system_cpu *cpu = &sys->hw.cpus[i];

		sha256_starts(&ctx);
		sha256_starts(&type);

		sha256_add(&ctx, &cpu->online);
		sha256_add(&ctx, &cpu->processor);
		sha256_add(&ctx, &cpu->core_id);
		sha256_add(&ctx, &cpu->physical_id);
		sha256_add(&ctx, &cpu->apicid);
		sha256_add(&ctx, &cpu->apicid_initial);
		sha256_add(&type, &cpu->cores);
		sha256_add(&type, &cpu->siblings);
		sha256_add(&ctx, &cpu->cpuid_level);
		sha256_add(&type, &cpu->cache_size);
		sha256_add(&type, &cpu->clflush_size);
		sha256_add(&type, &cpu->cache_alignment);
		sha256_add(&type, &cpu->family);
		sha256_add(&type, &cpu->model);
		sha256_add(&type, &cpu->stepping);
		sha256_add(&type, &cpu->fpu);
		sha256_add(&type, &cpu->fpu_exception);
		sha256_add(&type, &cpu->wp);
		sha256_add_str(&type, cpu->vendor_id);
		sha256_add_str(&type, cpu->model_name);
		sha256_add_str(&type, cpu->microcode);
		sha256_add_str(&type, cpu->address_sizes_str);
		sha256_add_str(&type, cpu->power_management);
		sha256_add_str(&type, cpu->flags);

		sha256_add(&ctx, &cpu->cpufreq);
		if (cpu->cpufreq) {
			if (!strcmp("userspace", cpu->governor))
				sha256_add(&ctx, &cpu->cur_freq);

			sha256_add(&ctx, &cpu->min_freq);
			sha256_add(&ctx, &cpu->max_freq);
			sha256_add_str(&ctx, cpu->avail_freq);
		}

		sha256_finish_str(&type, cpu->type_sha256);
		sha256_add_str(&ctx, cpu->type_sha256);
		sha256_finish_str(&ctx, cpu->sha256);

		sha256_add_str(&ctx_all, cpu->sha256);
	}

	sha256_finish_str(&ctx_all, sys->hw.cpus_sha256);
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
	if (sys->custom_info)
		sha256_add_str(&ctx, sys->custom_info);

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


int system_info_init(struct system *sys, const char *custom_info)
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
	sys->custom_info = custom_info;

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

#define system_info_fields 11
const struct header *system_info_hdr(struct system *sys)
{
	static const struct header hdr[system_info_fields + 1] = {
		{
			.name = "custom_info",
			.description = "User provided system information",
		},
		{ .name = "machine" },
		{ .name = "kernel" },
		{ .name = "nr_cpus" },
		{ .name = "nr_cpus_on" },
		{ .name = "nr_cores_on" },
		{ .name = "physical_cores_on" },
		{ .name = "mem_total" },
		{ .name = "mem_totalhigh" },
		{ .name = "mem_unit" },
		{ .name = "swap_total" },
		{ /* Sentinel */ }
	};
	return hdr;
}

struct data *system_info_data(struct system *sys)
{
	struct data *d = data_alloc(DATA_TYPE_OTHER, system_info_fields);

	if (!d)
		return NULL;

	data_add_str(d, sys->custom_info);
	data_add_str(d, sys->machine);
	data_add_str(d, sys->kernel_release);
	data_add_int32(d, sys->hw.nr_cpus);
	data_add_int32(d, sys->hw.nr_cpus_on);
	data_add_int32(d, sys->hw.nr_cores_on);
	data_add_int32(d, sys->hw.nr_physical_procs_on);
	data_add_int64(d, sys->hw.mem.mem_total);
	data_add_int64(d, sys->hw.mem.mem_totalhigh);
	data_add_int64(d, sys->hw.mem.mem_unit);
	data_add_int64(d, sys->hw.mem.swap_total);

	return d;
}

#define system_cpu_fields 14
const struct header *system_cpu_hdr(struct system_cpu *cpu)
{
	static const struct header hdr[system_cpu_fields + 1] = {
		{ .name = "processor" },
		{ .name = "online" },
		{ .name = "core_id" },
		{ .name = "physical_id" },
		{ .name = "apicid" },
		{ .name = "apicid_initial" },
		{ .name = "cpuid_level" },
		{ .name = "mhz" },
		{ .name = "bogomips" },
		{ .name = "governor" },
		{ .name = "avail_freq" },
		{ .name = "min_freq" },
		{ .name = "max_freq" },
		{ .name = "cur_freq" },
		{ /* Sentinel */ },
	};

	return hdr;
}

struct data *system_cpu_data(struct system_cpu *cpu)
{
	struct data *d = data_alloc(DATA_TYPE_OTHER, system_cpu_fields);

	if (!d)
		return NULL;

	data_add_int32(d, cpu->processor);
	data_add_int32(d, cpu->online);
	data_add_int32(d, cpu->core_id);
	data_add_int32(d, cpu->physical_id);
	data_add_int32(d, cpu->apicid);
	data_add_int32(d, cpu->apicid_initial);
	data_add_int32(d, cpu->cpuid_level);
	data_add_double(d, cpu->mhz);
	data_add_double(d, cpu->bogomips);
	data_add_str(d, (cpu->cpufreq ? cpu->governor : "none"));
	data_add_str(d, (cpu->cpufreq ? cpu->avail_freq : ""));
	data_add_int64(d, cpu->min_freq);
	data_add_int64(d, cpu->max_freq);
	data_add_int64(d, cpu->cur_freq);

	return d;
}

#define system_cpu_type_fields 17
const struct header *system_cpu_type_hdr(struct system_cpu *cpu)
{
	static const struct header hdr[system_cpu_type_fields + 1] = {
		{ .name = "cores" },
		{ .name = "siblings" },
		{ .name = "cache_size" },
		{ .name = "clflush_size" },
		{ .name = "cache_alignment" },
		{ .name = "family" },
		{ .name = "model" },
		{ .name = "stepping" },
		{ .name = "fpu" },
		{ .name = "fpu_exception" },
		{ .name = "wp" },
		{ .name = "vendor_id" },
		{ .name = "model_name" },
		{ .name = "microcode" },
		{ .name = "address_sizes" },
		{ .name = "power_management" },
		{ .name = "flags" },
		{ /* Sentinel */ }
	};

	return hdr;
}

struct data *system_cpu_type_data(struct system_cpu *cpu)
{
	struct data *d = data_alloc(DATA_TYPE_OTHER, system_cpu_type_fields);

	if (!d)
		return NULL;

	data_add_int32(d, cpu->cores);
	data_add_int32(d, cpu->siblings);
	data_add_int32(d, cpu->cache_size);
	data_add_int32(d, cpu->clflush_size);
	data_add_int32(d, cpu->cache_alignment);
	data_add_int32(d, cpu->family);
	data_add_int32(d, cpu->model);
	data_add_int32(d, cpu->stepping);
	data_add_int32(d, cpu->fpu);
	data_add_int32(d, cpu->fpu_exception);
	data_add_int32(d, cpu->wp);
	data_add_str(d, cpu->vendor_id);
	data_add_str(d, cpu->model_name);
	data_add_str(d, cpu->microcode);
	data_add_str(d, cpu->address_sizes_str);
	data_add_str(d, cpu->power_management);
	data_add_str(d, cpu->flags);

	return d;
}

