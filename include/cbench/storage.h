#ifndef _CBENCH_STORAGE_H_
#define _CBENCH_STORAGE_H_

struct data;
struct plugin;
struct system;

struct storage_ops {
	void *(*init)(const char *path);
	int (*init_plugin_grp)(void *storage, struct list_head *plugins,
				const char *sha256);
	int (*add_sysinfo)(void *storage, struct system *sys);
	int (*add_data)(void *storage, struct plugin *plug, struct data *data);
	int (*exit_plugin_grp)(void *storage);
	void (*exit)(void *storage);
};

struct storage {
	const struct storage_ops *ops;

	void *data;
};

static inline int storage_init_plg_grp(struct storage *storage,
		struct list_head *plugins, const char *sha256)
{
	if (!storage->ops->init_plugin_grp)
		return 0;
	return storage->ops->init_plugin_grp(storage->data, plugins, sha256);
}
static inline int storage_add_sysinfo(struct storage *storage, struct system *sys)
{
	if (!storage->ops->add_sysinfo)
		return 0;
	return storage->ops->add_sysinfo(storage->data, sys);
}
static inline int storage_add_data(struct storage *storage, struct plugin *plug,
		struct data *data)
{
	if (!storage->ops->add_data)
		return 0;
	return storage->ops->add_data(storage->data, plug, data);
}
static inline void storage_exit_plg_grp(struct storage *storage)
{
	if (!storage->ops->exit_plugin_grp)
		return;
	storage->ops->exit_plugin_grp(storage->data);
}

static inline int storage_init(struct storage *storage,
				const struct storage_ops *ops, const char *path)
{
	storage->data = ops->init(path);
	if (!storage->data)
		return -1;
	storage->ops = ops;
	return 0;
}

static inline void storage_exit(struct storage *storage)
{
	storage->ops->exit(storage->data);
}

#endif  /* _CBENCH_STORAGE_H_ */
