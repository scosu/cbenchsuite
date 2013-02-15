#ifndef _CBENCH_STORAGE_H_
#define _CBENCH_STORAGE_H_

struct data;
struct plugin;
struct system;

struct storage {
	void *(*init)(const char *path);
	int (*add_sysinfo)(void *storage, struct system *sys);
	int (*add_data)(void *storage, struct plugin *plug, struct data *data);
	void (*exit)(void *storage);
};

#endif  /* _CBENCH_STORAGE_H_ */
