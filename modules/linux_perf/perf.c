
#include <unistd.h>

#include <cbench/exec_helper.h>
#include <cbench/module.h>
#include <cbench/plugin.h>
#include <cbench/requirement.h>
#include <cbench/version.h>

extern const struct plugin_id plugin_hackbench;
extern const struct plugin_id plugin_sched_pipe;

static const struct plugin_id *plugins[] = {
	&plugin_hackbench,
	&plugin_sched_pipe,
	NULL
};

struct module_id perf_module = {
	.plugins = plugins,
};
MODULE_REGISTER(perf_module);
