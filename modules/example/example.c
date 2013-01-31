
#include <stdlib.h>
#include <stdio.h>

#include <benchsuite.h>
#include <module.h>
#include <options.h>
#include <plugin.h>
#include <version.h>

#include <modules/example/options.h>

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
		.version = "1.1",
		.comp_versions = example_bench_comp_ver,
		.data = NULL,
	}, {
		.version = "1.2",
		.comp_versions = example_bench_comp_ver,
	}, {

	}
};

static struct plugin_id example_plugs[] = {
	{
		.name = "example-bench",
		.versions = example_bench_ver,
		.data = NULL,

		.install = NULL,
		.init_pre = NULL,
		.init = NULL,
		.init_post = NULL,
		.run_pre = NULL,
		.run = NULL,
		.run_post = NULL,
		.parse_results = NULL,
		.monitor = NULL,
		.exit_pre = NULL,
		.exit = NULL,
		.exit_post = NULL,
		.check_stderr = NULL,
		.uninstall = NULL,
	}, {
		.name = "mon",
		.versions = example_bench_ver,
	}, {

	}
};


static struct plugin_link plug_grp_1[] = {
	{ .name = "example.example-bench" },
	{ .name = "example.example-monitor" },
	{ .name = "example.example-bgload", .add_instances = 4 },
	{ }
};

static struct plugin_link *example_plugin_groups[] = {
	plug_grp_1,
};

static struct benchsuite_id example_suites[] = {
	{
		.name = "example-suite",
		.plugin_grps = example_plugin_groups,
	}, {

	}
};

static int example_init(struct module *mod)
{
	printf("example init function\n");
	return 0;
}

static int example_exit(struct module *mod)
{
	return 0;
}

static int example_plug_init(struct plugin *plug)
{
	return 0;
}

static int example_plug_free(struct plugin *plug)
{
	return 0;
}

static int example_parser(struct plugin *plug, const char *optstr)
{
	struct option_iterator itr;
	struct example_options *opts = malloc(sizeof(*opts));
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

