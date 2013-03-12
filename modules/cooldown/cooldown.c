
#include <unistd.h>

#include <cbench/exec_helper.h>
#include <cbench/module.h>
#include <cbench/plugin.h>
#include <cbench/requirement.h>
#include <cbench/version.h>

enum plugins {
	PLUGIN_SLEEP = 0,
};

struct module_id cooldown_module;

#include "cooldown_sleep.c"

static struct plugin_id plugins[] = {
	{
		.name = "sleep",
		.description = "Helper plugin. Sleeps a given time to make sure that the system had some time to cool down.",
		PLUGIN_ALL_FUNCS(sleep_func),
		.versions = plugin_sleep_versions,
	}, {
		/* Sentinel */
	}
};

struct module_id cooldown_module = {
	.plugins = plugins,
};
MODULE_REGISTER(cooldown_module);
