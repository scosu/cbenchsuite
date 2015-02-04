#ifndef _CBENCH_PLUGIN_H_
#define _CBENCH_PLUGIN_H_

#include <stddef.h>

#include <klib/list.h>

#include <cbench/data.h>
#include <cbench/module.h>

struct data;
struct environment;
struct plugin_id;
struct version;

enum called_func {
	PLUGIN_CALLED_INIT_PRE,
	PLUGIN_CALLED_INIT,
	PLUGIN_CALLED_INIT_POST,
	PLUGIN_CALLED_RUN_PRE,
	PLUGIN_CALLED_RUN,
	PLUGIN_CALLED_RUN_POST,
	PLUGIN_CALLED_PARSE_RESULTS,
	PLUGIN_CALLED_EXIT_PRE,
	PLUGIN_CALLED_EXIT,
	PLUGIN_CALLED_EXIT_POST,
};

struct plugin {
	const struct plugin_id *id;
	struct module *mod;
	const struct version *version;

	char sha256[65];
	char opt_sha256[65];
	char ver_sha256[65];

	char *work_dir;
	const char *download_dir;

	void *plugin_data;
	void *version_data;
	struct header *options;

	void *user_data;

	/* Do not change this, it's used for execution */
	void *exec_data;

	enum called_func called_fun;

	struct list_head plugins;
	struct list_head plugin_grp;

	struct list_head run_data;
	struct list_head check_err_data;
};

struct plugin_id {
	char *name;
	char *description;
	struct version *versions;

	void *data;

	int (*module_init)(struct module *mod, const struct plugin_id *plug);
	void (*module_exit)(struct module *mod, const struct plugin_id *plug);

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

	const struct header* (*data_hdr)(struct plugin *plug);

	int (*monitor)(struct plugin *plug);
	int (*check_stderr)(struct plugin *plug);
};

static inline const struct version *plugin_get_version(struct plugin *plug)
{
	return plug->version;
}
static inline void *plugin_get_version_data(struct plugin *plug)
{
	return plug->version_data;
}
static inline const struct plugin_id *plugin_get_plugin_id(struct plugin *plug)
{
	return plug->id;
}
static inline void *plugin_get_plugin_data(struct plugin *plug)
{
	return plug->plugin_data;
}
static inline const char *plugin_get_bin_path(struct plugin *plug)
{
	return plug->mod->bin_path;
}
static inline const char *plugin_get_work_dir(struct plugin *plug)
{
	return plug->work_dir;
}
static inline const char *plugin_get_download_dir(struct plugin *plug)
{
	return plug->download_dir;
}

static inline const struct header *plugin_get_options(struct plugin *plug)
{
	return plug->options;
}

static inline const struct header *plugin_data_hdr(struct plugin *plug)
{
	if (!plug->id->data_hdr)
		return NULL;
	return plug->id->data_hdr(plug);
}

static inline void *plugin_get_data(struct plugin *plug)
{
	return plug->user_data;
}

static inline void plugin_set_data(struct plugin *plug, void *data)
{
	plug->user_data = data;
}

static inline void plugin_add_results(struct plugin *plug, struct data *data)
{
	INIT_LIST_HEAD(&data->run_data);
	list_add_tail(&data->run_data, &plug->run_data);
}

int plugins_execute(struct environment *env, struct list_head *plugins,
		const char *status_prefix);

void plugin_calc_sha256(struct plugin *plug);

void plugin_id_print(const struct plugin_id *plug, int verbose);

int plugin_version_check_requirements(const struct plugin_id *plug,
		const struct version *ver);

#define plugin_for_each_result(plugin, data) \
	list_for_each_entry(data, &(plugin)->check_err_data, run_data)

#endif  /* _CBENCH_PLUGIN_H_ */
