#ifndef _CBENCH_CORE_MODULE_MANAGER_H_
#define _CBENCH_CORE_MODULE_MANAGER_H_

#include <klib/list.h>

struct mod_mgr {
	char *module_dir;
	struct list_head modules;
};


int mod_mgr_init(struct mod_mgr *mm, const char *mod_dir);

void mod_mgr_unload_unused(struct mod_mgr *mm);

struct plugin *mod_mgr_plugin_create(struct mod_mgr *mm, const char *fid,
		const char *options, const char **ver_restrictions);

void mod_mgr_plugin_free(struct mod_mgr *mm, struct plugin *plug);

struct benchsuite *mod_mgr_benchsuite_create(struct mod_mgr *mm,
		const char *fid, const char **ver_restrictions);

void mod_mgr_benchsuite_free(struct mod_mgr *mm, struct benchsuite *suite);

void mod_mgr_exit(struct mod_mgr *mm);

#endif  /* _CBENCH_CORE_MODULE_MANAGER_H_ */
