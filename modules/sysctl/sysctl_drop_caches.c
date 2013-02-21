
#include <stdio.h>
#include <stdlib.h>

#include <cbench/option.h>

static struct requirement plugin_drop_caches_requirements[] = {
	{
		.name = vm_drop_caches,
		.description = "This plugin needs write permissions for this file to drop caches",
		.found = 0,
	},
	{ }
};


#define DROP_CACHES_OPT_FUNC_RUN(opt_name) 				\
	{ 								\
		.name = opt_name, 					\
		.value = { .type = VALUE_INT32, .v_int32 = 0 }, 	\
	},

struct option plugin_drop_caches_defaults[] = {
	DROP_CACHES_OPT_FUNC_RUN("init_pre")
	DROP_CACHES_OPT_FUNC_RUN("init")
	DROP_CACHES_OPT_FUNC_RUN("init_post")
	DROP_CACHES_OPT_FUNC_RUN("run_pre")
	DROP_CACHES_OPT_FUNC_RUN("run")
	DROP_CACHES_OPT_FUNC_RUN("run_post")
	DROP_CACHES_OPT_FUNC_RUN("parse_results")
	DROP_CACHES_OPT_FUNC_RUN("exit_pre")
	DROP_CACHES_OPT_FUNC_RUN("exit")
	DROP_CACHES_OPT_FUNC_RUN("exit_post")
	{
		.value = VALUE_STATIC_SENTINEL
	}
};

#undef DROP_CACHES_OPT_FUNC_RUN

static struct version plugin_drop_caches_versions[] = {
	{
		.version = "0.1",
		.requirements = plugin_drop_caches_requirements,
		.default_options = plugin_drop_caches_defaults,
	},
	{
		/* Sentinel */
	}
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

#define DROP_CACHES_FUNC(name) 						\
	static int drop_caches_##name(struct plugin *plug) 		\
	{ 								\
		const struct option *opts = plugin_get_options(plug); 	\
		if (option_get_int32(opts, #name)) 			\
			return drop_caches(); 				\
		return 0; 						\
	}

DROP_CACHES_FUNC(init_pre)
DROP_CACHES_FUNC(init)
DROP_CACHES_FUNC(init_post)
DROP_CACHES_FUNC(run_pre)
DROP_CACHES_FUNC(run)
DROP_CACHES_FUNC(run_post)
DROP_CACHES_FUNC(parse_results)
DROP_CACHES_FUNC(exit_pre)
DROP_CACHES_FUNC(exit)
DROP_CACHES_FUNC(exit_post)

#undef DROP_CACHES_FUNC
