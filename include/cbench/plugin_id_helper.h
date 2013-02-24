#ifndef _INCLUDE_CBENCH_PLUGIN_ID_HELPER_H_
#define _INCLUDE_CBENCH_PLUGIN_ID_HELPER_H_

#include <cbench/option.h>
#include <cbench/plugin.h>

#define OPTION_INT32(opt_name, def_val) \
	{ .name = opt_name, .value = { .type = VALUE_INT32, .v_int32 = def_val }}

#define OPTION_INT64(opt_name, def_val) \
	{ .name = opt_name, .value = { .type = VALUE_INT64, .v_int64 = def_val }}

#define OPTION_STR(opt_name, def_val) \
	{ .name = opt_name, .value = { .type = VALUE_STRING, .v_str = def_val }}

#define OPTION_BOOL(opt_name, def_val) \
		OPTION_INT32(opt_name, def_val)

/* Define one option for every general purpose execution function available,
 * add a prefix to the option */
#define OPTIONS_BOOL_EACH_FUNC_PRE(prefix, def_val) \
	OPTION_BOOL(prefix "init_pre", def_val), \
	OPTION_BOOL(prefix "init", def_val), \
	OPTION_BOOL(prefix "init_post", def_val), \
	OPTION_BOOL(prefix "run_pre", def_val), \
	OPTION_BOOL(prefix "run", def_val), \
	OPTION_BOOL(prefix "run_post", def_val), \
	OPTION_BOOL(prefix "parse_results", def_val), \
	OPTION_BOOL(prefix "exit_pre", def_val), \
	OPTION_BOOL(prefix "exit", def_val), \
	OPTION_BOOL(prefix "exit_post", def_val)

/* Define one option for every general purpose execution function available */
#define OPTIONS_BOOL_EACH_FUNC(def_val) \
		OPTIONS_BOOL_EACH_FUNC_PRE("", def_val)

#define OPTION_SENTINEL { .value = VALUE_STATIC_SENTINEL }

/* Returns != 0 if the currently executed function has an equivalent option
 * that is set to != 0. This can help you to create plugins with variable
 * slot execution, for example to clean systems or wait for something. */
static inline int plugin_exec_func_opt_set(struct plugin *plug)
{
	const struct option *opts = plugin_get_options(plug);
	switch (plug->called_fun) {
	case PLUGIN_CALLED_INIT_PRE:
		return option_get_int32(opts, "init_pre");
	case PLUGIN_CALLED_INIT:
		return option_get_int32(opts, "init");
	case PLUGIN_CALLED_INIT_POST:
		return option_get_int32(opts, "init_post");
	case PLUGIN_CALLED_RUN_PRE:
		return option_get_int32(opts, "run_pre");
	case PLUGIN_CALLED_RUN:
		return option_get_int32(opts, "run");
	case PLUGIN_CALLED_RUN_POST:
		return option_get_int32(opts, "run_post");
	case PLUGIN_CALLED_PARSE_RESULTS:
		return option_get_int32(opts, "parse_results");
	case PLUGIN_CALLED_EXIT_PRE:
		return option_get_int32(opts, "exit_pre");
	case PLUGIN_CALLED_EXIT:
		return option_get_int32(opts, "exit");
	case PLUGIN_CALLED_EXIT_POST:
		return option_get_int32(opts, "exit_post");
	default:
		return 0;
	}
}


/* Set all slot function pointer to func_ptr */
#define PLUGIN_ALL_FUNCS(func_ptr) \
		.init_pre = func_ptr,\
		.init = func_ptr,\
		.init_post = func_ptr,\
		.run_pre = func_ptr,\
		.run = func_ptr,\
		.run_post = func_ptr,\
		.parse_results = func_ptr,\
		.exit_pre = func_ptr,\
		.exit = func_ptr,\
		.exit_post = func_ptr

#endif  /* _INCLUDE_CBENCH_PLUGIN_ID_HELPER_H_ */
