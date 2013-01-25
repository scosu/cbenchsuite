#ifndef _MODULE_H_
#define _MODULE_H_

struct list_head;
struct module_id;
struct plugin;

struct module {
	struct module_id *id;

	void *user_data;
};

struct module_id {
	const char *name;

	const struct plugin_id *plugins;
	const struct benchsuite *benchsuites;

	int (*prepare)(struct module *mod);
	int (*init)(struct module *mod);
	int (*exit)(struct module *mod);

	int (*plugin_init)(struct plugin *plug);
	int (*plugin_free)(struct plugin *plug);

	struct list_head *modules;
};


#define MODULE_REGISTER(mod) \
	__attribute__((unused)) const struct module_id *__module__ = &(mod);


struct run_settings {
	int runtime_min;
	int runtime_max;
	int runs_min;
	int runs_max;
};

#endif  /* _MODULE_H_ */
