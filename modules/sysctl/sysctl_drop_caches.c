
#include <stdio.h>
#include <stdlib.h>

#include <options.h>

static struct requirement plugin_drop_caches_requirements[] = {
	{
		.name = vm_drop_caches,
		.description = "This plugin needs this file to drop caches",
		.found = 0,
	},
	{ }
};

static struct version plugin_drop_caches_versions[] = {
	{
		.version = "0.1",
		.requirements = plugin_drop_caches_requirements,
	},
	{ }
};

static int drop_caches()
{
	FILE *fd;
	int ret = 0;

	fd = fopen(vm_drop_caches, "w");
	if (fd == NULL)
		return 1;
	ret = fwrite("3\n", 1, 2, fd);
	fclose(fd);
	if (ret != 2)
		return 1;
	return 0;
}

static int plugin_drop_caches_parser(struct plugin *plug, const char *opts)
{
	struct option_iterator opt;
	struct options_sysctl_drop_caches *opt_data = malloc(sizeof(*opt_data));
	int flags = 0;

	if (!opt_data)
		return 1;

	options_for_each_entry(&opt, opts) {
		int enabled = option_parse_bool(&opt);
		if (!enabled)
			continue;

		if (!option_key_cmp(&opt, "init_pre"))
			flags |= OPTS_SYSCTL_DROP_CACHES_INIT_PRE;
		else if (!option_key_cmp(&opt, "init"))
			flags |= OPTS_SYSCTL_DROP_CACHES_INIT;
		else if (!option_key_cmp(&opt, "init_post"))
			flags |= OPTS_SYSCTL_DROP_CACHES_INIT_POST;
		else if (!option_key_cmp(&opt, "run_pre"))
			flags |= OPTS_SYSCTL_DROP_CACHES_RUN_PRE;
		else if (!option_key_cmp(&opt, "run"))
			flags |= OPTS_SYSCTL_DROP_CACHES_RUN;
		else if (!option_key_cmp(&opt, "run_post"))
			flags |= OPTS_SYSCTL_DROP_CACHES_RUN_POST;
		else if (!option_key_cmp(&opt, "parse_results"))
			flags |= OPTS_SYSCTL_DROP_CACHES_PARSE_RESULTS;
		else if (!option_key_cmp(&opt, "exit_pre"))
			flags |= OPTS_SYSCTL_DROP_CACHES_EXIT_PRE;
		else if (!option_key_cmp(&opt, "exit"))
			flags |= OPTS_SYSCTL_DROP_CACHES_EXIT;
		else if (!option_key_cmp(&opt, "exit_post"))
			flags |= OPTS_SYSCTL_DROP_CACHES_EXIT_POST;
	}

	opt_data->active_functions = flags;
	plugin_set_options(plug, opt_data);
	printf("flags: %d\n", flags);

	return 0;
}

#define DROP_CACHES_FUNC(opt, name) 					\
	static int drop_caches_##name(struct plugin *plug) 		\
	{ 								\
		printf("%s\n", __func__); 				\
		struct options_sysctl_drop_caches *opts = plugin_get_options(plug);\
		if (opts->active_functions & OPTS_SYSCTL_DROP_CACHES_##opt)\
			return drop_caches(); 				\
		return 0; 						\
	}

DROP_CACHES_FUNC(INIT_PRE, init_pre)
DROP_CACHES_FUNC(INIT, init)
DROP_CACHES_FUNC(INIT_POST, init_post)
DROP_CACHES_FUNC(RUN_PRE, run_pre)
DROP_CACHES_FUNC(RUN, run)
DROP_CACHES_FUNC(RUN_POST, run_post)
DROP_CACHES_FUNC(PARSE_RESULTS, parse_results)
DROP_CACHES_FUNC(EXIT_PRE, exit_pre)
DROP_CACHES_FUNC(EXIT, exit)
DROP_CACHES_FUNC(EXIT_POST, exit_post)

#undef DROP_CACHES_FUNC
