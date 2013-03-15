
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

#include "bench_dc_sqrt.c"
#include "bench_dhry.c"
#include "bench_whet.c"
#include "bench_linpack.c"

static int mod_init()
{
	char *which_args[] = {"which", NULL, NULL};
	int ret;
	which_args[1] = "dc";

	ret = subproc_call("which", which_args);
	if (!ret)
		plugin_dc_sqrt_requirements[0].found = 1;
	return 0;
}

static struct plugin_id plugins[] = {
	{
		.name = "dc-sqrt",
		.description = "Benchmark to calculate the square root to a fixed number of digits using dc",
		.install = dc_sqrt_install,
		.uninstall = dc_sqrt_uninstall,
		.run = dc_sqrt_run,
		.data_hdr = dc_sqrt_data_hdr,
		.versions = plugin_dc_sqrt_versions,
	}, {
		.name = "dhrystone",
		.description = "Dhrystone benchmark",
		.install = dhry_install,
		.uninstall = dhry_uninstall,
		.run = dhry_run,
		.data_hdr = dhry_data_hdr,
		.versions = plugin_dhry_versions,
	}, {
		.name = "linpack",
		.description = "Linpack benchmark",
		.install = linpack_install,
		.uninstall = linpack_uninstall,
		.run = linpack_run,
		.data_hdr = linpack_data_hdr,
		.versions = plugin_linpack_versions,
	}, {
		.name = "whetstone",
		.description = "Whetone benchmark",
		.install = whet_install,
		.uninstall = whet_uninstall,
		.run = whet_run,
		.data_hdr = whet_data_hdr,
		.versions = plugin_whet_versions,
	}, {
		/* Sentinel */
	}
};

struct module_id perf_module = {
	.init = mod_init,
	.plugins = plugins,
};
MODULE_REGISTER(perf_module);
