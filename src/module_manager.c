
#include <core/module_manager.h>

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

#include <benchsuite.h>
#include <module.h>
#include <plugin.h>
#include <version.h>


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

	err = mod->id->init(mod);
	if (err) {
		printk(KERN_ERR "Failed initializing module %s returncode %d\n",
				mod->name, err);
		goto failed_init_module;
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
		printk(KERN_ERR "Full identifier is not valid: %s\n", fid);
		return NULL;
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
				selected = &plug_ids[i];
				selected_version = &vers[j];
				goto found_matching_plugin;
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
	mod_mgr_module_put_plugin(mm, plg->mod, plg);
}

struct plugin *mod_mgr_plugin_create(struct mod_mgr *mm, const char *fid,
		const char **ver_restrictions)
{
	const char *plug_start;
	struct module *mod = mod_mgr_find_module(mm, fid);
	struct plugin *plug;

	plug_start = strchr(fid, '.');
	if (!plug_start) {
		printk(KERN_ERR "Full identifier is not valid: %s\n", fid);
		return NULL;
	}
	++plug_start;

	plug = mod_mgr_module_get_plugin(mm, mod, plug_start, ver_restrictions);
	if (!plug)
		return NULL;
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
