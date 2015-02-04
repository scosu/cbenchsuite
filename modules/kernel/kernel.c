
#include <cbench/exec_helper.h>
#include <cbench/module.h>
#include <cbench/plugin.h>
#include <cbench/version.h>

#include "versions.c"

int kernel_wget(struct plugin *plug)
{
	enum kernel_versions ver =
			(enum kernel_versions)plugin_get_version_data(plug);
	return cbench_wget(plug, kernels[ver].url, kernels[ver].file_name);
}

int kernel_wget_untar(struct plugin *plug, char *dst)
{
	enum kernel_versions ver =
			(enum kernel_versions)plugin_get_version_data(plug);
	return cbench_wget_untar(plug, kernels[ver].url, kernels[ver].file_name, dst,
			NULL);
}

extern const struct plugin_id plugin_compile;

static const struct plugin_id *kernel_plugins[] = {
	&plugin_compile,
	NULL
};

static struct module_id kernel_module_id = {
	.plugins = kernel_plugins
};
MODULE_REGISTER(kernel_module_id);
