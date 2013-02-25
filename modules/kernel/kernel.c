
#include <cbench/config.h>
#include <cbench/exec_helper.h>
#include <cbench/module.h>
#include <cbench/plugin.h>
#include <cbench/version.h>

struct kernel_version {
	char *version;
	char *name;
	char *file_name;
	char *url;
};

#define KERNEL_VERSION(maj, min) KERNEL_##maj##_##min,
enum kernel_versions {
#include "versions.h"
};
#undef KERNEL_VERSION

#define KERNEL_VERSION(maj, min) \
	{ .version = #maj "." #min, .file_name = "linux-" #maj "." #min ".tar.gz",\
	  .name = "linux-" #maj "." #min,\
	  .url = CONFIG_KERNEL_MIRROR "/linux-" #maj "." #min ".tar.gz" },

static struct kernel_version kernels[] = {
#include "versions.h"
	{ /* Sentinel */ }
};
#undef KERNEL_VERSION

#define KERNEL_VERSION(maj, min) {\
	{ .name = "kernel", .version = #maj "." #min },\
	{ }\
},
struct comp_version kernel_comp_versions[][2] = {
#include "versions.h"
};
#undef KERNEL_VERSION

static int kernel_wget(struct plugin *plug)
{
	enum kernel_versions ver =
			(enum kernel_versions)plugin_get_version_data(plug);
	return cbench_wget(plug, kernels[ver].url, kernels[ver].file_name);
}

static int kernel_wget_untar(struct plugin *plug, char *dst)
{
	enum kernel_versions ver =
			(enum kernel_versions)plugin_get_version_data(plug);
	return cbench_wget_untar(plug, kernels[ver].url, kernels[ver].file_name, dst,
			NULL);
}

#include "compile.c"

static struct plugin_id kernel_plugins[] = {
	kernel_compile_plugin,
	{ /* Sentinel */ }
};

static struct module_id kernel_module_id = {
	.plugins = kernel_plugins
};
MODULE_REGISTER(kernel_module_id);
