
#include <stdlib.h>
#include <stdio.h>

#include <benchsuite.h>
#include <module.h>
#include <options.h>
#include <plugin.h>
#include <version.h>

#include <modules/example/options.h>

static int plug_install(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}
static int plug_init_pre(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}
static int plug_init(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}
static int plug_init_post(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}
static int plug_run_pre(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}
static int plug_run(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}
static int plug_run_post(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}
static int plug_parse_results(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}
static int plug_monitor(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}
static int plug_exit_pre(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}
static int plug_exit(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}
static int plug_exit_post(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}
static int plug_check_stderr(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}
static int plug_uninstall(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}

static struct comp_version example_bench_comp_ver[] = {
	{
		.name = "component1",
		.version = "19.9",
	}, {
		.name = "component2",
		.version = "0.9",
	}, {

	}
};

static struct version example_bench_ver[] = {
	{
		.version = "1.2",
		.comp_versions = example_bench_comp_ver,
	}, {
		.version = "1.1",
		.comp_versions = example_bench_comp_ver,
		.data = NULL,
	}, {

	}
};

static struct plugin_id example_plugs[] = {
	{
		.name = "bench",
		.versions = example_bench_ver,
		.data = NULL,

		.install = plug_install,
		.init_pre = plug_init_pre,
		.init = plug_init,
		.init_post = plug_init_post,
		.run_pre = plug_run_pre,
		.run = plug_run,
		.run_post = plug_run_post,
		.parse_results = plug_parse_results,
		.monitor = plug_monitor,
		.exit_pre = plug_exit_pre,
		.exit = plug_exit,
		.exit_post = plug_exit_post,
		.check_stderr = plug_check_stderr,
		.uninstall = plug_uninstall,
	}, {
		.name = "mon",
		.versions = example_bench_ver,
	}, {

	}
};


static struct plugin_link plug_grp_1[] = {
	{ .name = "example.bench" },
	{ }
};

static struct plugin_link *example_plugin_groups[] = {
	plug_grp_1,
};

static struct benchsuite_id example_suites[] = {
	{
		.name = "suite",
		.plugin_grps = example_plugin_groups,
	}, {

	}
};

static int example_init(struct module *mod)
{
	printf("%s\n", __func__);
	return 0;
}

static int example_exit(struct module *mod)
{
	printf("%s\n", __func__);
	return 0;
}

static int example_plug_init(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}

static int example_plug_free(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}

static int example_parser(struct plugin *plug, const char *optstr)
{
	struct option_iterator itr;
	struct example_options *opts = malloc(sizeof(*opts));
	printf("%s\n", __func__);
	if (!opts)
		return 1;
	*opts = example_options_defaults;

	options_for_each_entry(&itr, optstr) {
		if (!option_key_cmp(&itr, "str")) {
			option_value_copy(&itr, opts->sample_str, 128);
		} else if (!option_key_cmp(&itr, "int")) {
			opts->sample_int = option_parse_int(&itr, 0, 100);
		} else if (!option_key_cmp(&itr, "bool")) {
			opts->sample_bool = option_parse_bool(&itr);
		}
	}
	plugin_set_options(plug, opts);
	return 0;
}



struct module_id example_mod = {
	.plugins = example_plugs,
	.benchsuites = example_suites,
	.prepare = NULL,
	.init = example_init,
	.exit = example_exit,
	.plugin_init = example_plug_init,
	.plugin_free = example_plug_free,
	.option_parser = example_parser,
};
MODULE_REGISTER(example_mod)

