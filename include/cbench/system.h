#ifndef _CBENCH_SYSTEM_H_
#define _CBENCH_SYSTEM_H_

#include <stddef.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

#include <klib/types.h>

/*
 * This version number has to be increased for every change to one of the
 * following structs. So you can use this number to get a warning as soon as
 * your storage backend becomes deprecated.
 */
#define SYSTEM_STRUCT_VERSION 1

struct system_raw {
	struct utsname uname;
	struct sysinfo sysinfo;
};

struct system_sw {
	const char *gcc;
	char *libc;
	char *libpthread;
};

struct system_cpu {
	int online;

	int processor;
	int core_id;
	int physical_id;
	int apicid;
	int apicid_initial;

	int cores;
	int siblings;
	int cpuid_level;

	int cache_size;
	int clflush_size;
	int cache_alignment;
	double mhz;
	double bogomips;

	int family;
	int model;
	int stepping;
	int fpu;
	int fpu_exception;
	int wp;

	int cpufreq;
	char *governor;
	char *avail_freq;
	int min_freq;
	int max_freq;
	int cur_freq;

	char *vendor_id;
	char *model_name;
	char *microcode;
	char *address_sizes_str;
	char *power_management;
	char *flags;
	char sha256[65];
	char type_sha256[65];
};

struct system_memory {
	u64 mem_total;
	u64 mem_totalhigh;
	u64 mem_unit;
	u64 swap_total;
};

struct system_hw {
	int nr_cpus;
	int nr_cpus_on;
	int nr_cores_on;
	int nr_physical_procs_on;
	char cpus_sha256[65];
	struct system_cpu *cpus;
	struct system_memory mem;
};

struct system {
	struct system_raw raw;

	struct system_hw hw;
	struct system_sw sw;

	const char *machine;
	const char *kernel_release;
	char sha256[65];
};

int system_info_init(struct system *sys);
void system_info_free(struct system *sys);

const struct value *system_info_hdr(struct system *sys);
struct data *system_info_data(struct system *sys);
const struct value *system_cpu_type_hdr(struct system_cpu *cpu);
struct data *system_cpu_type_data(struct system_cpu *cpu);
const struct value *system_cpu_hdr(struct system_cpu *cpu);
struct data *system_cpu_data(struct system_cpu *cpu);

#endif  /* _CBENCH_SYSTEM_H_ */
