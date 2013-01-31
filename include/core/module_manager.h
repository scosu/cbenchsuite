
#include <klib/list.h>

struct mod_mgr {
	char *module_dir;
	struct list_head modules;
};


int mod_mgr_init(struct mod_mgr *mm, const char *mod_dir);

int mod_mgr_unload_modules();

struct plugin *mod_mgr_plugin_create(struct mod_mgr *mm, const char *fid,
		const char **ver_restrictions);

void mod_mgr_plugin_free(struct mod_mgr *mm, struct plugin *plug);

void mod_mgr_exit(struct mod_mgr *mm);
