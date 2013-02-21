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

#include <cbench/core/module_manager.h>

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <klib/list.h>
#include <klib/printk.h>

#include <cbench/benchsuite.h>
#include <cbench/module.h>
#include <cbench/option.h>
#include <cbench/plugin.h>
#include <cbench/version.h>


static const char *module_so_name = "module.so";

int module_load(struct module *mod)
{
	char *err;
	struct module_id **mod_id;
	if (mod->so_handle)
		return 0;

	mod->so_handle = dlopen(mod->so_path, RTLD_NOW | RTLD_LOCAL);
	if (!mod->so_handle) {
		printk(KERN_ERR "Failed loading shared object module %s: %s\n",
				mod->name, dlerror());
		return -1;
	}

	dlerror();
	mod_id = dlsym(mod->so_handle, "__module__");
	err = dlerror();
	if (err) {
		printk(KERN_ERR "Failed to find module, did you register your "
				"module with MODULE_REGISTER? %s: %s\n",
				mod->name, err);
		dlclose(mod->so_handle);
		mod->so_handle = NULL;
		mod->id = NULL;
		return -1;
	}
	mod->id = *mod_id;
	return 0;
}

void module_unload(struct module *mod)
{
	if (!mod->so_handle || !list_empty(&mod->plugins))
		return;

	mod->id = NULL;

	dlclose(mod->so_handle);
	mod->so_handle = NULL;
}

struct module *module_create(const char *name, const char *mod_dir)
{
	struct module *mod;
	int name_len;
	int err;

	mod = malloc(sizeof(*mod));
	if (!mod)
		return NULL;
	memset(mod, 0, sizeof(*mod));

	name_len = strlen(name);
	mod->name = malloc(name_len + 1);
	if (!mod->name)
		goto failed_alloc_name;
	strcpy(mod->name, name);

	mod->so_path = malloc(name_len * 2 + strlen(mod_dir) + 6);
	if (!mod->so_path)
		goto failed_alloc_so_path;
	sprintf(mod->so_path, "%s/%s/%s", mod_dir, name, module_so_name);

	err = module_load(mod);
	if (err)
		goto failed_load_module;

	if (mod->id->init) {
		err = mod->id->init(mod);
		if (err) {
			printk(KERN_ERR "Failed initializing module %s returncode %d\n",
					mod->name, err);
			goto failed_init_module;
		}
	}

	INIT_LIST_HEAD(&mod->modules);
	INIT_LIST_HEAD(&mod->plugins);

	return mod;

failed_init_module:
	module_unload(mod);
failed_load_module:
	free(mod->so_path);
failed_alloc_so_path:
	free(mod->name);
failed_alloc_name:
	free(mod);
	return NULL;
}

void module_free(struct module *mod)
{
	module_load(mod);
	if (mod->id->exit) {
		mod->id->exit(mod);
	}
	module_unload(mod);
	free(mod->so_path);
	free(mod->name);
	free(mod);
}

int mod_mgr_init(struct mod_mgr *mm, const char *mod_dir)
{
	DIR *md;
	struct dirent *de;
	struct module *mod;
	const struct plugin_id *plug;
	int i;

	INIT_LIST_HEAD(&mm->modules);

	md = opendir(mod_dir);

	if (!md) {
		printk(KERN_ERR "Failed to open module directory %s: %s\n",
				mod_dir, strerror(errno));
		return errno;
	}

	while ((de = readdir(md))) {
		if (de->d_type != DT_DIR) {
			printk(KERN_DEBUG "%s is no directory, continuing\n",
					de->d_name);
			continue;
		}
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;

		mod = module_create(de->d_name, mod_dir);
		if (!mod)
			continue;

		list_add_tail(&mod->modules, &mm->modules);

		plug = mod->id->plugins;
		for (i = 0; plug[i].name != NULL; ++i) {
			printk(KERN_DEBUG "Plugin %s\n", plug[i].name);
		}
	}
	closedir(md);
	return 0;
}

struct module *mod_mgr_find_module(struct mod_mgr *mm, const char *fid)
{
	const char *mod_start = fid;
	int mod_len;
	const char *mod_end;
	struct module *mod;

	mod_end = strchr(mod_start, '.');
	if (!mod_end) {
		printk(KERN_WARNING "Not valid as full identifier: %s\n", fid);
		mod_end = mod_start + strlen(mod_start);
	}

	mod_len = mod_end - mod_start;

	list_for_each_entry(mod, &mm->modules, modules) {
		if (strlen(mod->name) != mod_len)
			continue;
		if (strncmp(mod->name, mod_start, mod_len))
			continue;

		return mod;
	}
	return NULL;
}

void mod_mgr_print_module(struct module *mod, int verbose)
{
	int i;
	printf("Module '%s'\n", mod->name);
	if (verbose)
		printf("  shared object '%s'\n", mod->so_path);
	if (mod->id->plugins) {
		printf("  Plugins:\n");
		for (i = 0; mod->id->plugins[i].name; ++i) {
			plugin_id_print(&mod->id->plugins[i], verbose);
		}
	}
	if (mod->id->benchsuites) {
		printf("  Benchsuites:\n");
		for (i = 0; mod->id->benchsuites[i].name; ++i) {
			benchsuite_id_print(&mod->id->benchsuites[i], verbose);
		}
	}
}

int mod_mgr_list_module(struct mod_mgr *mm, const char *fid, int verbose)
{
	struct module *mod;
	if (!fid) {
		list_for_each_entry(mod, &mm->modules, modules) {
			mod_mgr_print_module(mod, verbose);
		}
	} else {
		mod = mod_mgr_find_module(mm, fid);
		if (!mod) {
			printk(KERN_ERR "Did not find module %s\n", fid);
			return -1;
		}
		mod_mgr_print_module(mod, verbose);
	}
	return 0;
}

struct plugin *mod_mgr_module_get_plugin(struct mod_mgr *mm,
		struct module *mod, const char *plug_name,
		const char **ver_restrictions)
{
	const struct plugin_id *plug_ids = mod->id->plugins;
	const struct plugin_id *selected;
	struct plugin *plug;
	const struct version *selected_version;
	int i;

	printk(KERN_DEBUG "Module get plugin %s\n", plug_name);

	for (i = 0; plug_ids[i].name; ++i) {
		struct version *vers = plug_ids[i].versions;
		int j;
		int ret;
		if (strcmp(plug_ids[i].name, plug_name))
			continue;

		for (j = 0; vers[j].version != NULL; ++j) {
			ret = version_matching(&vers[j], ver_restrictions);
			if (ret) {
				ret = plugin_version_check_requirements(
						&plug_ids[i], &vers[j]);
				if (ret) {
					printk(KERN_WARNING "Missing requirements, trying another version\n");
				} else {
					selected = &plug_ids[i];
					selected_version = &vers[j];
					goto found_matching_plugin;
				}
			}
		}
	}

	printk(KERN_ERR "Did not find matching plugin\n");
	return NULL;

found_matching_plugin:
	printk(KERN_DEBUG "Module found matching plugin\n");
	plug = malloc(sizeof(*plug));
	if (!plug) {
		printk(KERN_ERR "Out of memory?!\n");
		return NULL;
	}
	memset(plug, 0, sizeof(*plug));

	plug->id = selected;
	plug->plugin_data = selected->data;
	plug->version_data = selected_version->data;
	plug->version = selected_version;
	plug->mod = mod;

	INIT_LIST_HEAD(&plug->plugins);
	INIT_LIST_HEAD(&plug->plugin_grp);
	INIT_LIST_HEAD(&plug->run_data);

	list_add_tail(&plug->plugins, &mod->plugins);

	printk(KERN_DEBUG "Module %s created plugin %s version %s\n",
			mod->name, selected->name, selected_version->version);

	return plug;
}

void mod_mgr_module_put_plugin(struct mod_mgr *mm, struct module *mod,
		struct plugin *plug)
{
	list_del(&plug->plugins);
	free(plug);
}

void mod_mgr_plugin_free(struct mod_mgr *mm, struct plugin *plg)
{
	if (plg->options)
		free(plg->options);
	if (plg->mod->id->plugin_free) {
		int ret = plg->mod->id->plugin_free(plg);
		if (ret) {
			printk(KERN_ERR "Failed to free plugin in module free function%s\n",
					plg->id->name);
		}
	}
	mod_mgr_module_put_plugin(mm, plg->mod, plg);
}

struct plugin *mod_mgr_plugin_create(struct mod_mgr *mm, const char *fid,
		const char *options, const char **ver_restrictions)
{
	const char *plug_start;
	struct module *mod = mod_mgr_find_module(mm, fid);
	struct plugin *plug;

	if (!mod) {
		printk(KERN_ERR "Failed to get module %s\n", fid);
		return NULL;
	}

	plug_start = strchr(fid, '.');
	if (!plug_start) {
		printk(KERN_ERR "Full identifier is not valid: %s\n", fid);
		return NULL;
	}
	++plug_start;

	plug = mod_mgr_module_get_plugin(mm, mod, plug_start, ver_restrictions);
	if (!plug)
		return NULL;

	plug->options = option_parse(plug->version->default_options, options);
	if (!plug->options) {
		printk(KERN_ERR "Option parsing error\n");
		mod_mgr_module_put_plugin(mm, mod, plug);
		return NULL;
	}

	plugin_calc_sha256(plug);

	if (mod->id->plugin_init) {
		int ret = mod->id->plugin_init(plug);
		if (ret) {
			printk(KERN_ERR "Failed to prepare plugin in module init function%s\n", fid);
			mod_mgr_module_put_plugin(mm, mod, plug);
			return NULL;
		}
	}

	return plug;
}

struct benchsuite *mod_mgr_module_get_benchsuite(struct mod_mgr *mm,
		struct module *mod, const char *bench_name,
		const char **ver_restrictions)
{
	const struct benchsuite_id *benchsuites = mod->id->benchsuites;
	const struct benchsuite_id *selected;
	struct benchsuite *suite;
	int i;

	printk(KERN_DEBUG "Module get benchsuite %s\n", bench_name);

	for (i = 0; benchsuites[i].name; ++i) {
		int ret;
		if (strcmp(benchsuites[i].name, bench_name))
			continue;
		ret = version_matching(&benchsuites[i].version, ver_restrictions);
		if (ret) {
			selected = &benchsuites[i];
			goto found_matching_benchsuite;
		}
	}

	printk(KERN_ERR "Did not find matching benchsuite\n");
	return NULL;

found_matching_benchsuite:
	printk(KERN_DEBUG "Module found matching plugin\n");

	suite = malloc(sizeof(*suite));
	if (!suite) {
		printk(KERN_ERR "Out of memory\n");
		return NULL;
	}
	memset(suite, 0, sizeof(*suite));

	suite->id = selected;
	suite->mod = mod;

	++mod->benchsuites_in_use;

	printk(KERN_DEBUG "Module %s found benchsuite %s version %s\n",
			mod->name, selected->name, selected->version);

	return suite;
}

void mod_mgr_module_put_benchsuite(struct mod_mgr *mm, struct benchsuite *suite)
{
	--suite->mod->benchsuites_in_use;
	free(suite);
}


struct benchsuite *mod_mgr_benchsuite_create(struct mod_mgr *mm,
		const char *fid, const char **ver_restrictions)
{
	const char *suite_start;
	struct module *mod = mod_mgr_find_module(mm, fid);
	struct benchsuite *suite;

	if (!mod) {
		printk(KERN_ERR "Failed to get module %s\n", fid);
		return NULL;
	}

	suite_start = strchr(fid, '.');
	if (!suite_start) {
		printk(KERN_ERR "Full identifier is not valid: %s\n", fid);
		return NULL;
	}
	++suite_start;

	suite = mod_mgr_module_get_benchsuite(mm, mod, suite_start, ver_restrictions);
	if (!suite)
		return NULL;
	return suite;
}

void mod_mgr_benchsuite_free(struct mod_mgr *mm, struct benchsuite *suite)
{
	mod_mgr_module_put_benchsuite(mm, suite);
}

void mod_mgr_unload_unused(struct mod_mgr *mm)
{
	struct module *mod;

	list_for_each_entry(mod, &mm->modules, modules) {
		if (!list_empty(&mod->plugins))
			module_unload(mod);
	}
}

void mod_mgr_exit(struct mod_mgr *mm)
{
	struct module *mod;
	struct module *modn;
	list_for_each_entry_safe(mod, modn, &mm->modules, modules) {
		list_del(&mod->modules);
		module_free(mod);
	}
}

