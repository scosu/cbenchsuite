
#include <unistd.h>

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
	if (!access(vm_drop_caches, F_OK | W_OK))
		sysctl_module.plugins[PLUGIN_DROP_CACHES].versions[0].requirements[0].found = 1;
	return 0;
}

static struct plugin_id plugins[] = {
	{
		.name = "drop-caches",
		PLUGIN_ALL_FUNCS(drop_caches_func),
		.versions = plugin_drop_caches_versions,
	},
	{ }
};

struct module_id sysctl_module = {
	.init = module_init,
	.plugins = plugins,
};
MODULE_REGISTER(sysctl_module);
