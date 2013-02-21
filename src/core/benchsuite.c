/*
 * Cbench - A C benchmarking suite for Linux benchmarking.
 * Copyright (C) 2013  Markus Pargmann <mpargmann@allfex.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <cbench/benchsuite.h>

#include <stdio.h>

#include <klib/list.h>
#include <klib/printk.h>

#include <cbench/core/module_manager.h>
#include <cbench/plugin.h>

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

void benchsuite_id_print(const struct benchsuite_id *suite, int verbose)
{
	printf("      %s (Version %s)\n", suite->name,
			suite->version.version ? suite->version.version : "none");
	if (verbose && suite->description) {
		printf("        %s\n", suite->description);
	}
}
