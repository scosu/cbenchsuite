#ifndef _CBENCH_PLUGIN_H_
#define _CBENCH_PLUGIN_H_

#include <stddef.h>

#include <klib/list.h>

struct data;
struct environment;
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
	void *exec_data;

	struct list_head plugins;
	struct list_head plugin_grp;

	struct list_head run_data;
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

	void (*free_data)(struct plugin *plug, struct data *data);

	int (*data_to_comma_str)(struct plugin *plug, struct data *data, char **buf,
			size_t *buf_len);
	int (*data_to_comma_hdr)(struct plugin *plug, struct data *data, char **buf,
			size_t *buf_len);

	int (*monitor)(struct plugin *plug);
	int (*check_stderr)(struct plugin *plug);

	int (*option_parser)(struct plugin *plug, const char *opts);
};

static inline void plugin_set_options(struct plugin *plug, void *options)
{
	plug->options = options;
}
static inline void *plugin_get_options(struct plugin *plug)
{
	return plug->options;
}

static inline int plugin_data_comma_str(struct plugin *plug, struct data *data,
					char **buf, size_t *buf_len)
{
	if (!plug->id->data_to_comma_str)
		return -1;
	return plug->id->data_to_comma_str(plug, data, buf, buf_len);
}
static inline int plugin_data_comma_hdr(struct plugin *plug, struct data *data,
					char **buf, size_t *buf_len)
{
	if (!plug->id->data_to_comma_hdr)
		return -1;
	return plug->id->data_to_comma_hdr(plug, data, buf, buf_len);
}

void plugin_add_data(struct plugin *plug, void *data);

int plugins_execute(struct environment *env, struct list_head *plugins);

#define plugin_parse_options(plug, opt_str) \
	(plug)->mod->id->option_parser(plug, opt_str);

#endif  /* _CBENCH_PLUGIN_H_ */
