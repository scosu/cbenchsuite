#ifndef _CBENCH_PLUGIN_H_
#define _CBENCH_PLUGIN_H_

#include <stddef.h>

#include <klib/list.h>

#include <cbench/data.h>

struct data;
struct environment;
struct module;
struct plugin_id;
struct version;

struct plugin {
	const struct plugin_id *id;
	struct module *mod;
	const struct version *version;

	char sha256[65];
	char opt_sha256[65];

	const char *bin_path;
	const char *work_dir;

	void *plugin_data;
	void *version_data;
	void *options;
	void *exec_data;

	void *ctx;

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

	const struct value* (*data_hdr)(struct plugin *plug);

	int (*monitor)(struct plugin *plug);
	int (*check_stderr)(struct plugin *plug);
};

static inline const struct option *plugin_get_options(struct plugin *plug)
{
	return plug->options;
}

static inline const struct value *plugin_data_hdr(struct plugin *plug)
{
	if (!plug->id->data_hdr)
		return NULL;
	return plug->id->data_hdr(plug);
}

static inline void *plugin_get_data(struct plugin *plug)
{
	return plug->ctx;
}

static inline void plugin_set_data(struct plugin *plug, void *data)
{
	plug->ctx = data;
}

static inline void plugin_add_results(struct plugin *plug, struct data *data)
{
	INIT_LIST_HEAD(&data->run_data);
	list_add_tail(&data->run_data, &plug->run_data);
}

int plugins_execute(struct environment *env, struct list_head *plugins);

void plugin_calc_sha256(struct plugin *plug);

void plugin_id_print(const struct plugin_id *plug, int verbose);

int plugin_version_check_requirements(const struct plugin_id *plug,
		const struct version *ver);

#define plugin_for_each_result(plugin, data) \
	list_for_each_entry(data, &(plugin)->check_err_data, run_data)

#endif  /* _CBENCH_PLUGIN_H_ */
