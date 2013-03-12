
#include <unistd.h>

#include <cbench/exec_helper.h>
#include <cbench/module.h>
#include <cbench/plugin.h>
#include <cbench/requirement.h>
#include <cbench/version.h>

enum plugins {
	PLUGIN_DROP_CACHES = 0,
	PLUGIN_SWAP_RESET,
};

const char vm_drop_caches[] = "/proc/sys/vm/drop_caches";

struct module_id sysctl_module;

#include "sysctl_drop_caches.c"
#include "sysctl_swap_reset.c"

static int module_init(struct module *mod)
{
	char *check_swap_args[] = {"which", "swapoff", "swapon", NULL};
	if (!access(vm_drop_caches, F_OK | W_OK))
		sysctl_module.plugins[PLUGIN_DROP_CACHES].versions[0].requirements[0].found = 1;
	if (!subproc_call("which", check_swap_args))
		sysctl_module.plugins[PLUGIN_SWAP_RESET].versions[0].requirements[0].found = 1;
	return 0;
}

static struct plugin_id plugins[] = {
	{
		.name = "drop-caches",
		.description = "Helper plugin to increase benchmarking accuracy by dropping caches between runs. You have configure in which function slot the caches are dropped.",
		PLUGIN_ALL_FUNCS(drop_caches_func),
		.versions = plugin_drop_caches_versions,
	}, {
		.name = "swap-reset",
		.description = "Helper plugin to increase benchmarking accuracy by resetting the swap between runs. You have to configure in which function slot the swap is reset. Make sure your swap is configured in fstab so that swapoff/swapon doesn't  change the swap setup.",
		PLUGIN_ALL_FUNCS(swapreset_func),
		.versions = plugin_swapreset_versions,
	}, {
		/* Sentinel */
	}
};

struct module_id sysctl_module = {
	.init = module_init,
	.plugins = plugins,
};
MODULE_REGISTER(sysctl_module);
