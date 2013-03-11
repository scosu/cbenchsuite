
#include <stdio.h>
#include <stdlib.h>

#include <cbench/option.h>
#include <cbench/plugin_id_helper.h>

static struct requirement plugin_swapreset_requirements[] = {
	{
		.name = "swapoff/swapon",
		.description = "This plugin uses swapoff/on to reset swap.",
		.found = 0,
	},
	{ }
};

struct header plugin_swapreset_defaults[] = {
	OPTIONS_BOOL_EACH_FUNC(0),
	OPTION_SENTINEL
};

static struct version plugin_swapreset_versions[] = {
	{
		.version = "0.1",
		.requirements = plugin_swapreset_requirements,
		.default_options = plugin_swapreset_defaults,
	},
	{
		/* Sentinel */
	}
};

static int swapreset()
{
	char *swapoff[] = {"swapoff", "-a", NULL};
	char *swapon[] = {"swapon", "-a", NULL};
	int ret;

	ret = subproc_call("swapoff", swapoff);
	ret |= subproc_call("swapon", swapon);

	return ret;
}

static int swapreset_func(struct plugin *plug)
{
	if (!plugin_exec_func_opt_set(plug))
		return 0;
	return swapreset();
}
