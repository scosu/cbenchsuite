
#include <stdlib.h>
#include <stdio.h>

#include <cbench/benchsuite.h>
#include <cbench/data.h>
#include <cbench/module.h>
#include <cbench/option.h>
#include <cbench/plugin.h>
#include <cbench/version.h>

/*
 * ============================================================================
 * Plugin definition
 *
 * For explanations, see at the end of file.
 */

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
	struct data *data = data_alloc(DATA_TYPE_RESULT, 1);

	data_add_float(data, 42.0);
	plugin_add_results(plug, data);

	printf("%s\n", __func__);
	return 0;
}
static const struct value* plug_data_hdr(struct plugin *plug)
{
	static const struct value hdr[] = {
		{ .v_str = "Example header for first field" },
		VALUE_STATIC_SENTINEL
	};

	return hdr;
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
	float sum = 0;
	struct data *d;

	plugin_for_each_result(plug, d) {
		sum += data_get_float(d, 0);
	}
	printf("%s%f\n", __func__, sum);

	/*
	 * Do some calculations here...
	 * and decide if stderr is reached (1) or not (0)
	 */

	return 1;
}
static int plug_uninstall(struct plugin *plug)
{
	printf("%s\n", __func__);
	return 0;
}
static void plug_stop(struct plugin *plug)
{
	printf("%s\n", __func__);
}

/*
 * Default options. An option can be int32, int64 or string. Options will be
 * automatically parsed and passed to the created plugin object.
 *
 * Required fields:
 *  - name: Name of the option.
 *  - value type: Type of the value, one of VALUE_STRING, VALUE_INT32, VALUE_INT64
 *  - value v_int32,v_int64,v_str: Default value.
 */
static struct option example_default_options[] = {
	{ .name = "opt-1" , .value = { .type = VALUE_INT32, .v_int32 = 1 }, },
	{ .value = VALUE_STATIC_SENTINEL }
};

/*
 * Component versions. Every external component that is used by this plugin
 * has to be listed here, so cbench can seperate results with different versions.
 *
 * Required fields:
 *  - name: component name
 *  - version: component version
 */
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

/*
 * Versions for an example plugin. One of them will be selected when a plugin is
 * created. plugin->version will point on the selected version.
 *
 * Required fields:
 *  - version: Main version of the plugin as string. It should be ints seperated
 *  	with dots, so the cbench version matcher can parse it.
 *  - nr_independent_values: If your plugin produces results (not monitors) you
 *  	have set this to the number of independently measured values you get
 *  	from a single run. For example compress/decompress time are independent,
 *  	but runtime and variance of the runtime are dependent.
 *
 * Optional fields:
 *  - requirements: List of requirements
 *  - default_options: Options of this plugin with default values
 *  - comp_versions: Versions of the used components.
 *  - data: Version data, will be set in plugin->version_data
 */
static struct version example_bench_ver[] = {
	{
		.version = "1.2",
		.comp_versions = example_bench_comp_ver,
		.nr_independent_values = 1,
	}, {
		.version = "1.1",
		.comp_versions = example_bench_comp_ver,
		.data = NULL,
		.nr_independent_values = 1,
		.default_options = example_default_options,
	}, {
		/* Sentinel */
	}
};


/*
 * Plugin definition. A plugin in general can do different things. Some
 * of the possibilities are:
 *  - Monitor: Implement a monitor function. It is called every second while
 *  	run() is executed.
 *  - Benchmark: Implement a run function where the benchmark is executed.
 *  	Finally return the results in parse_results.
 *  - Background load: Implement a run which generated system load or something
 *  	else and stops when stop() is called.
 *  - Environment setup: Setup some environment variables in one of the init/exit
 *  	functions, or wait for a system state to start benchmarking. Also
 *  	cleaning caches etc. is possible.
 *
 * Required fields:
 *  - name: A module-wide unique name for this plugin.
 *  - versions: Array of available plugin versions.
 *
 * Optional fields:
 *  - description: Description of the plugin
 *  - data: If this plugin is created, the plugin object will have a copy of this
 *  	pointer as 'plugin_data'.
 *  - init_pre/init/init_post/run_pre/run_post/parse_results/exit_pre/
 *  	exit/exit_post are function pointer that are executed in this order
 *  	for every run.
 *  - run: Run functions that create background load or do something not
 *  	benchmark related, should run until stop() is called. This is important
 *  	because the first terminating run() process will trigger a stop() for all
 *  	other plugins.
 *  - install/uninstall: Are called once before using the plugin and after using
 *  	it. There is a newly created working directory at plugin->work_dir.
 *  	Do not use changedir or similar. The directory is removed after uninstall.
 *  - stop: Called after the first plugin run() finished.
 *  - data_hdr: If your plugin produces data, return a static header here. The
 *  	header memory will not be freed by cbench. In case you want to change
 *  	the header and the previous header was already in use, you have to
 *  	change the plugin version.
 *  - monitor: Called once a second when run is executed.
 *  - check_stderr: Custom calculations if the standard error was reached. If
 *  	this function is not implemented, cbench will calculate directly on
 *  	the data measured.
 */
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
		.exit_pre = plug_exit_pre,
		.exit = plug_exit,
		.exit_post = plug_exit_post,
		.uninstall = plug_uninstall,

		.monitor = plug_monitor,

		.data_hdr = plug_data_hdr,
		.stop = plug_stop,
		.check_stderr = plug_check_stderr,
	}, {
		.name = "mon",
		.versions = example_bench_ver,
	}, {

	}
};



/*
 * ============================================================================
 * Example benchsuite definition starting here
 */

/*
 * Example of version rules. You can limit the version numbers for specific
 * components of the plugin, if the plugin sets them.
 * See doc/identifier_specification.txt for more information.
 */
static const char *example_version_rules[] = {
	">=1.0",
	"glib<2.0",
	NULL /* Sentinel */
};

/*
 * Definition of one plugin group, consisting of an array of plugin links.
 * A plugin link is resolved at runtime so there can't be any load dependencies
 * between modules.
 *
 * A plugin_link has the following fields:
 * Required fields:
 *  - name: A full identifier of a plugin <MODULE>.<PLUGIN>.
 *
 * Optional fields:
 *  - options: A string with plugin options, seperated by ':'.
 *  	See doc/identifier_specification.txt
 *  - version_rules: Version rules for a plugin. If there is no matching plugin
 *  	version, this plugin group will cause an abort.
 */
static struct plugin_link plug_grp_1[] = {
	{ .name = "example.bench", },
	{ .name = "sysctl.drop_caches", .options = "run_pre=1",
		.version_rules = example_version_rules, },
	{ /* Sentinel */ }
};


/*
 * Array of plugin group definitions for the benchsuite. A plugin group is
 * itself an array of plugins which are executed in parallel.
 * The list of plugin groups is executed sequentially.
 */
static struct plugin_link *example_plugin_groups[] = {
	plug_grp_1,
	NULL /* Sentinel */
};



/*
 * Array of benchsuites defined by this module.
 * After the last real item of the array, add an empty array item, so cbench
 * knows where the end of the array is.
 *
 * Required fields:
 *  - name: A module-wide unique name to identify this benchsuite.
 *  - plugin_grps: Pointer to an array of plugin groups that should
 *  	be executed
 *
 * Optional fields:
 *  - version: Version struct defining the version of the benchsuite.
 *  	You should not use comp_versions here, just define .version.
 *  - description: Description of the benchsuite and what it does.
 */
static struct benchsuite_id example_suites[] = {
	{
		.name = "suite",
		.plugin_grps = example_plugin_groups,
		.version = { .version = "1.0", },
		.description = "This is an example benchsuite",
	}, {
		/* Sentinel */
	}
};


/*
 * ============================================================================
 * Module definitions
 */


/*
 * Some sample functions for the function pointer of modules.
 * A return value = 0 means success, return != 0 is error
 */
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

/*
 * A module_id contains all information about the module. Only one definition
 * that is registered with MODULE_REGISTER, is allowed.
 *
 * There are no required fields for this struct.
 *
 * Optional fields:
 *  - plugins: Pointer to an array of plugin_id definitions. This defines all
 *  	available plugins.
 *  - benchsuite: Pointer to an array of benchsuite_id definitions. All available
 *  	benchsuites.
 *  - init: Function pointer to a init function that is called on module load.
 *  	Be aware, that a module may be loaded multiple times in the runtime of
 *  	cbench. All data except for the user_data of the module is destroyed
 *  	after module unloading.
 *  - exit: Function pointer to a exit function that is called on module unload.
 *  	Be aware, that a module may be loaded multiple times in the runtime of
 *  	cbench. All data except for the user_data of the module is destroyed
 *  	after module unloading.
 *  - plugin_init: Function pointer that is called when a new plugin was created.
 *  - plugin_exit: Function pointer that is called when a plugin gets deleted.
 */
struct module_id example_mod = {
	.plugins = example_plugs,
	.benchsuites = example_suites,
	.init = example_init,
	.exit = example_exit,
	.plugin_init = example_plug_init,
	.plugin_free = example_plug_free,
};
MODULE_REGISTER(example_mod);
/*
 * DO NOT FORGET MODULE_REGISTER, or your module will not be detected by cbench.
 */

