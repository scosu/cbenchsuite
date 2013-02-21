#include <cbench/module.h>
#include <cbench/plugin.h>

#include "yield.c"


static const struct plugin_id cpusched_plugins[] = {
	yield_bench_plugin_id
	{
		/* Sentinel */
	}
};

static const struct module_id cpusched_mod = {
	.plugins = cpusched_plugins,
};
MODULE_REGISTER(cpusched_mod);