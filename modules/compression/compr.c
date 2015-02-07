
#include <unistd.h>

#include <cbench/module.h>
#include <cbench/plugin.h>
#include <cbench/requirement.h>
#include <cbench/version.h>

extern const struct plugin_id plugin_7zip;

static const struct plugin_id *plugins[] = {
	&plugin_7zip,
	NULL
};

struct module_id perf_module = {
	.plugins = plugins,
};
MODULE_REGISTER(perf_module);
