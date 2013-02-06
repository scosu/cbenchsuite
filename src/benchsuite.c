
#include <benchsuite.h>

#include <klib/list.h>
#include <klib/printk.h>

#include <core/module_manager.h>
#include <plugin.h>

int benchsuite_execute(struct mod_mgr *mm, struct environment *env,
		struct benchsuite *suite)
{
	int i;
	int ret = 0;
	for (i = 0; suite->id->plugin_grps[i] != NULL
			&& suite->id->plugin_grps[i]->name != NULL; ++i) {
		struct plugin_link *grp = suite->id->plugin_grps[i];
		int j;
		struct list_head pgrp;
		struct plugin *plg, *nplg;

		INIT_LIST_HEAD(&pgrp);

		for (j = 0; grp[j].name != NULL; ++j) {
			plg = mod_mgr_plugin_create(mm,
					grp[j].name,
					grp[j].options,
					grp[j].version_rules);
			if (!plg) {
				printk(KERN_ERR "Didn't find plugin %s\n",
						grp[j].name);
				ret = 1;
				goto error_populating_group;
			}
			list_add_tail(&plg->plugin_grp, &pgrp);
		}

		mod_mgr_unload_unused(mm);

		ret = plugins_execute(env, &pgrp);

error_populating_group:
		list_for_each_entry_safe(plg, nplg, &pgrp, plugin_grp) {
			list_del(&plg->plugin_grp);
			mod_mgr_plugin_free(mm, plg);
		}
		if (ret)
			break;
	}
	return ret;
}
