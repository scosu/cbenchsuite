
#include <unistd.h>

#include <cbench/module.h>
#include <cbench/plugin.h>

extern const struct plugin_id plugin_sleep;

static const struct plugin_id *plugins[] = {
	&plugin_sleep,
	NULL
};

struct module_id cooldown_module = {
	.plugins = plugins,
};
MODULE_REGISTER(cooldown_module);
