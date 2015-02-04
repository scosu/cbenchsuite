
#include <unistd.h>

#include <cbench/exec_helper.h>
#include <cbench/module.h>
#include <cbench/plugin.h>
#include <cbench/requirement.h>
#include <cbench/version.h>

extern void dc_sqrt_init();

static int mod_init()
{
	dc_sqrt_init();
	return 0;
}

extern const struct plugin_id plugin_dc_sqrt;
extern const struct plugin_id plugin_dhry;
extern const struct plugin_id plugin_linpack;
extern const struct plugin_id plugin_whetstone;

static const struct plugin_id *plugins[] = {
	&plugin_dc_sqrt,
	&plugin_dhry,
	&plugin_linpack,
	&plugin_whetstone,
	NULL
};

struct module_id perf_module = {
	.init = mod_init,
	.plugins = plugins,
};
MODULE_REGISTER(perf_module);
