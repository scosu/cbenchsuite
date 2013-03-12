
#include <unistd.h>

#include <cbench/exec_helper.h>
#include <cbench/module.h>
#include <cbench/plugin.h>
#include <cbench/requirement.h>
#include <cbench/version.h>

enum plugins {
	PLUGIN_HACKBENCH = 0,
	PLUGIN_SCHED_PIPE,
};

struct module_id perf_module;

#include "perf_hackbench.c"
#include "sched-pipe.c"

static struct plugin_id plugins[] = {
	{
		.name = "hackbench",
		.description = "Benchmark that spawns a number of groups that internally send/receive packets. Also known within the linux kernel perf tool as sched-messaging.",
		.install = hackbench_install,
		.uninstall = hackbench_uninstall,
		.parse_results = hackbench_parse_results,
		.run = hackbench_run,
		.data_hdr = hackbench_data_hdr,
		.versions = plugin_hackbench_versions,
	}, {
		.name = "sched-pipe",
		.description = "Benchmarks the pipe performance by spawning two processes and sending data between them.",
		.run = sched_pipe_run,
		.data_hdr = sched_pipe_data_hdr,
		.versions = plugin_sched_pipe_versions,
	}, {
		/* Sentinel */
	}
};

struct module_id perf_module = {
	.plugins = plugins,
};
MODULE_REGISTER(perf_module);
