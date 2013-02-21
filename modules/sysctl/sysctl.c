
#include <stdio.h>

#include <cbench/module.h>
#include <cbench/plugin.h>
#include <cbench/requirement.h>
#include <cbench/version.h>

enum plugins {
	PLUGIN_DROP_CACHES = 0,
};

const char vm_drop_caches[] = "/proc/sys/vm/drop_caches";

struct module_id sysctl_module;

#include "sysctl_drop_caches.c"

static int module_init(struct module *mod)
{
	FILE *f = fopen(vm_drop_caches, "w");
	if (f) {
		sysctl_module.plugins[PLUGIN_DROP_CACHES].versions[0].requirements[0].found = 1;
		fclose(f);
	}
	return 0;
}

static struct plugin_id plugins[] = {
	{
		.name = "drop_caches",
		.init_pre = drop_caches_init_pre,
		.init = drop_caches_init,
		.init_post = drop_caches_init_post,
		.run_pre = drop_caches_run_pre,
		.run = drop_caches_run,
		.run_post = drop_caches_run_post,
		.parse_results = drop_caches_parse_results,
		.exit_pre = drop_caches_exit_pre,
		.exit = drop_caches_exit,
		.exit_post = drop_caches_exit_post,
		.versions = plugin_drop_caches_versions,
	},
	{ }
};

struct module_id sysctl_module = {
	.init = module_init,
	.plugins = plugins,
};
MODULE_REGISTER(sysctl_module);
