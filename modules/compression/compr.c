
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

#include "7zip.c"

static int mod_init()
{
	return p7zip_init_7z();
}

static int mod_exit()
{
	p7zip_exit_7z();
	return 0;
}

static struct plugin_id plugins[] = {
	{
		.name = "7zip-bench",
		.install = p7zip_bench_install,
		.uninstall = p7zip_bench_uninstall,
		.parse_results = p7zip_bench_parse_results,
		.run = p7zip_bench_run,
		.data_hdr = p7zip_bench_data_hdr,
		.versions = plugin_p7zip_bench_versions,
	}, {
		/* Sentinel */
	}
};

struct module_id perf_module = {
	.plugins = plugins,
	.init = mod_init,
	.exit = mod_exit,
};
MODULE_REGISTER(perf_module);
