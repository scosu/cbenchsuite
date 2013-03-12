
#include <stdio.h>
#include <stdlib.h>

#include <cbench/option.h>
#include <cbench/plugin_id_helper.h>
#include <unistd.h>

#define SLEEP_OPTION(func) \
	OPTION_INT32(func, "Sleep to cooldown the system in this function slot.", "s", 0)

struct header plugin_sleep_defaults[] = {
	SLEEP_OPTION("init_pre"),
	SLEEP_OPTION("init"),
	SLEEP_OPTION("init_post"),
	SLEEP_OPTION("run_pre"),
	SLEEP_OPTION("run"),
	SLEEP_OPTION("run_post"),
	SLEEP_OPTION("parse_results"),
	SLEEP_OPTION("exit_pre"),
	SLEEP_OPTION("exit"),
	SLEEP_OPTION("exit_post"),
	OPTION_SENTINEL
};
#undef SLEEP_OPTION

static struct version plugin_sleep_versions[] = {
	{
		.version = "0.1",
		.default_options = plugin_sleep_defaults,
	},
	{
		/* Sentinel */
	}
};

static int sleep_func(struct plugin *plug)
{
	int seconds = plugin_exec_func_opt_set(plug);
	if (!seconds)
		return 0;
	return sleep(seconds);
}

#undef DROP_CACHES_FUNC
