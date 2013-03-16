#include <cbench/module.h>
#include <cbench/plugin.h>

#include "yield.c"
#include "fork-bench.c"
#include "monitor_latency.c"
#include "suite_sched.c"


static const struct plugin_id cpusched_plugins[] = {
	yield_bench_plugin_id
	fork_bench_plugin_id
	monitor_latency_plugin_id
	{
		/* Sentinel */
	}
};

static const struct benchsuite_id mod_suites[] = {
	{
		.name = "sched-suite",
		.plugin_grps = suite_sched_groups,
		.version = { .version = "0.1", },
		.description = "A collection of scheduler benchmarks, while monitoring the scheduler behavior",
	}, {
		/* Sentinel */
	}
};

static const struct module_id cpusched_mod = {
	.plugins = cpusched_plugins,
	.benchsuites = mod_suites,
};
MODULE_REGISTER(cpusched_mod);
