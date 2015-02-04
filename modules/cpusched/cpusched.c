#include <cbench/module.h>
#include <cbench/plugin.h>

extern const struct plugin_id plugin_yield_bench;
extern const struct plugin_id plugin_fork_bench;
extern const struct plugin_id plugin_latency_monitor;
extern const struct benchsuite_id suite_normal;

static const struct plugin_id *cpusched_plugins[] = {
	&plugin_yield_bench,
	&plugin_fork_bench,
	&plugin_latency_monitor,
	NULL
};

static const struct benchsuite_id *mod_suites[] = {
	&suite_normal,
	NULL
};

static const struct module_id cpusched_mod = {
	.plugins = cpusched_plugins,
	.benchsuites = mod_suites,
};
MODULE_REGISTER(cpusched_mod);
