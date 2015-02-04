
#include <time.h>
#include <stdio.h>

#include <cbench/option.h>
#include <cbench/plugin_id_helper.h>
#include <cbench/exec_helper.h>

#include "versions.c"

int kernel_wget(struct plugin *plug);
int kernel_wget_untar(struct plugin *plug, char *dst);

struct kernel_compile_data {
	char *src_path;
	char threads_str[16];
	struct timespec start;
	struct timespec end;
};

static int kernel_compile_install(struct plugin *plug)
{
	struct kernel_compile_data *d;
	const struct header *opts = plugin_get_options(plug);
	const char *work_dir = plugin_get_work_dir(plug);
	enum kernel_versions ver =
			(enum kernel_versions)plugin_get_version_data(plug);
	int ret;

	plugin_set_data(plug, NULL);

	ret = kernel_wget(plug);
	if (ret)
		return ret;

	d = malloc(sizeof(*d));
	if (!d)
		return -1;

	d->src_path = malloc(strlen(work_dir) + strlen(kernels[ver].name) + 2);
	if (!d->src_path) {
		free(d);
		return -1;
	}

	sprintf(d->src_path, "%s/%s", work_dir, kernels[ver].name);
	sprintf(d->threads_str, "%d", option_get_int32(opts, "threads"));

	plugin_set_data(plug, d);
	return 0;
}

static int kernel_compile_uninstall(struct plugin *plug)
{
	struct kernel_compile_data *d = plugin_get_data(plug);

	if (!d)
		return 0;

	free(d->src_path);
	free(d);
	return 0;
}

static int kernel_compile_init(struct plugin *plug)
{
	struct kernel_compile_data *d = plugin_get_data(plug);
	char *make_args[] = {"make", "-C", d->src_path, "defconfig", NULL};
	int ret = cbench_mkdir(plug, "src");
	if (ret)
		return ret;

	ret = kernel_wget_untar(plug, ".");
	if (ret)
		return ret;

	ret = subproc_call("make", make_args);

	return 0;
}

static int kernel_compile_run(struct plugin *plug)
{
	struct kernel_compile_data *d = plugin_get_data(plug);
	char *make_args[] = {"make", "-C", d->src_path, "-j", d->threads_str, NULL};
	int ret;

	clock_gettime(CLOCK_MONOTONIC_RAW, &d->start);

	ret = subproc_call("make", make_args);

	clock_gettime(CLOCK_MONOTONIC_RAW, &d->end);

	return ret;
}

static int kernel_compile_parse_results(struct plugin *plug)
{
	struct kernel_compile_data *d = plugin_get_data(plug);
	struct data *dat = data_alloc(DATA_TYPE_RESULT, 1);

	double runtime = d->end.tv_sec - d->start.tv_sec;
	runtime += ((double)d->end.tv_nsec - (double)d->start.tv_nsec)/1000000000.0;

	data_add_double(dat, runtime);

	plugin_add_results(plug, dat);

	return 0;
}

static int kernel_compile_exit(struct plugin *plug)
{
	enum kernel_versions ver =
			(enum kernel_versions)plugin_get_version_data(plug);
	return cbench_rm_rec(plug, kernels[ver].name);
}

static const struct header *kernel_compile_data_hdr(struct plugin *plug)
{
	static const struct header hdr[] = {
		{
			.name = "runtime",
			.description = "Runtime of kernel build with default config.",
			.unit = "s",
			.data_type = DATA_LESS_IS_BETTER,
		}, {
			/* Sentinel */
		}
	};

	return hdr;
}

static struct header kernel_compile_options[] = {
	OPTION_INT32("threads", "Number of threads used to compile the kernel", NULL, 16),
	OPTION_SENTINEL,
};

#define KERNEL_VERSION(maj, min) {\
	.version = "1.0",\
	.data = (void*)KERNEL_##maj##_##min,\
	.nr_independent_values = 1,\
	.default_options = kernel_compile_options,\
	.comp_versions = kernel_comp_versions[KERNEL_##maj##_##min]\
},

static struct version kernel_compile_versions[] = {
#include "versions.h"
	{ /* Sentinel */ }
};
#undef KERNEL_VERSION

const struct plugin_id plugin_compile = {
	.name = "compile",
	.description = "Linux kernel compile benchmark. It measures how fast the kernel can be compiled",
	.install = kernel_compile_install,
	.uninstall = kernel_compile_uninstall,
	.init = kernel_compile_init,
	.exit = kernel_compile_exit,
	.run = kernel_compile_run,
	.parse_results = kernel_compile_parse_results,
	.versions = kernel_compile_versions,
	.data_hdr = kernel_compile_data_hdr,
};
