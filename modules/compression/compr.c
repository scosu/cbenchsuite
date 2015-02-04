
#include <unistd.h>

#include <cbench/module.h>
#include <cbench/plugin.h>
#include <cbench/requirement.h>
#include <cbench/version.h>

extern int p7zip_init_7z();
extern void p7zip_exit_7z();
extern const struct plugin_id plugin_7zip;

struct module_id perf_module;

static int mod_init()
{
	return p7zip_init_7z();
}

static int mod_exit()
{
	p7zip_exit_7z();
	return 0;
}

static const struct plugin_id *plugins[] = {
	&plugin_7zip,
	NULL
};

struct module_id perf_module = {
	.plugins = plugins,
	.init = mod_init,
	.exit = mod_exit,
};
MODULE_REGISTER(perf_module);
