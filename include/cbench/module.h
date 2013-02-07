#ifndef _CBENCH_MODULE_H_
#define _CBENCH_MODULE_H_

#include <klib/list.h>

struct module_id;
struct plugin;

struct module {
	char *name;
	struct module_id *id;

	char *so_path;
	void *so_handle;

	void *user_data;

	struct list_head modules;

	struct list_head plugins;
	int benchsuites_in_use;
};

struct module_id {
	const struct plugin_id *plugins;
	const struct benchsuite_id *benchsuites;

	int (*prepare)(struct module *mod);
	int (*init)(struct module *mod);
	int (*exit)(struct module *mod);

	int (*plugin_init)(struct plugin *plug);
	int (*plugin_free)(struct plugin *plug);
};


#define MODULE_REGISTER(mod) \
	__attribute__((unused)) const struct module_id *__module__ = &(mod);

static inline void *module_get_user_data(struct module *mod)
{
	return mod->user_data;
}

static inline void module_set_user_data(struct module *mod, void *data)
{
	mod->user_data = data;
}


#endif  /* _CBENCH_MODULE_H_ */
