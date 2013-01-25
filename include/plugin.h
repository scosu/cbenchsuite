#ifndef _PLUGIN_H_
#define _PLUGIN_H_

struct module;
struct plugin_id;
struct version;

struct plugin {
	struct plugin_id *id;
	struct module *mod;
	struct version *version;

	void *user_data;
	void *plugin_data;
	void *version_data;
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
	int (*deinstall)(struct plugin *plug);

	int (*monitor)(struct plugin *plug);
	int (*check_stderr)(struct plugin *plug);
};


#endif  /* _PLUGIN_H_ */
