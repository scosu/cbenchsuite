
#include <stdlib.h>

#include <benchsuite.h>
#include <module.h>
#include <plugin.h>
#include <version.h>

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
		.name = "1.1",
		.comp_versions = example_bench_comp_ver,
		.data = NULL,
	}, {
		.name = "1.2",
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
		.deinstall = NULL,
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

static struct benchsuite example_suites[] = {
	{
		.name = "example-suite",
		.plugin_grps = example_plugin_groups,
	}, {

	}
};

static int example_init(struct module *mod)
{
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

struct module_id example_mod = {
	.name = "example",
	.plugins = example_plugs,
	.benchsuites = example_suites,
	.prepare = NULL,
	.init = example_init,
	.exit = example_exit,
	.plugin_init = example_plug_init,
	.plugin_free = example_plug_free,
};
MODULE_REGISTER(example_mod)

