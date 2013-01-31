#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include <klib/list.h>

struct module;
struct plugin_id;
struct version;

struct plugin {
	const struct plugin_id *id;
	struct module *mod;
	const struct version *version;

	const char *bin_path;
	const char *work_dir;

	void *user_data;
	void *plugin_data;
	void *version_data;
	void *options;

	struct list_head plugins;
};

struct plugin_id {
	char *name;
	struct version *versions;

	void *data;

	int (*install)(struct plugin *plug);
	int (*init_pre)(struct plugin *plug);
	int (*init)(struct plugin *plug);
	int (*init_post)(struct plugin *plug);
	int (*run_pre)(struct plugin *plug);
	int (*run)(struct plugin *plug);
	int (*run_post)(struct plugin *plug);
	int (*parse_results)(struct plugin *plug);
	int (*exit_pre)(struct plugin *plug);
	int (*exit)(struct plugin *plug);
	int (*exit_post)(struct plugin *plug);
	int (*uninstall)(struct plugin *plug);

	void (*stop)(struct plugin *plug);

	int (*monitor)(struct plugin *plug);
	int (*check_stderr)(struct plugin *plug);
};

static inline void plugin_set_options(struct plugin *plug, void *options)
{
	plug->options = options;
}

#define plugin_parse_options(plug, opt_str) \
	(plug)->mod->id->option_parser(plug, opt_str);

struct run_settings {
	int warmup_runs;
	int runtime_min;
	int runtime_max;
	int runs_min;
	int runs_max;
	double percent_stderr;
};

#endif  /* _PLUGIN_H_ */
