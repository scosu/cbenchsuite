#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <cbench/option.h>
#include <cbench/plugin_id_helper.h>
#include <cbench/version.h>
#include <cbench/exec_helper.h>

static struct header plugin_linpack_options[] = {
	OPTION_INT32("loops", "Number of loops to execute", NULL, 500),
	OPTION_INT32("arraysize", "Size of the 2D array", NULL, 1000),
	OPTION_SENTINEL
};

static struct version plugin_linpack_versions[] = {
	{
		.version = "0.1",
		.nr_independent_values = 1,
		.default_options = plugin_linpack_options,
	}, {
		/* Sentinel */
	}
};

struct linpack_data {
	char *bin;
	char iterations[32];
	char arraysize[32];
	char *args[5];
};

static int linpack_install(struct plugin *plug)
{
	struct linpack_data *d = malloc(sizeof(*d));
	const char *bin_path = plugin_get_bin_path(plug);
	const struct header *opts = plugin_get_options(plug);

	if (!d)
		return -1;

	d->bin = malloc(strlen(bin_path) + 10);
	if (!d->bin)
		return -1;

	sprintf(d->bin, "%s/linpack", bin_path);

	sprintf(d->iterations, "%d", option_get_int32(opts, "loops"));
	sprintf(d->arraysize, "%d", option_get_int32(opts, "arraysize"));

	d->args[0] = d->bin;
	d->args[1] = d->arraysize;
	d->args[2] = d->iterations;
	d->args[3] = NULL;

	plugin_set_data(plug, d);

	return 0;
}

static int linpack_uninstall(struct plugin *plug)
{
	struct linpack_data *d = plugin_get_data(plug);

	if (!d)
		return 0;

	free(d->bin);
	free(d);

	plugin_set_data(plug, NULL);

	return 0;
}

static int linpack_run(struct plugin *plug)
{
	struct linpack_data *d = plugin_get_data(plug);
	int ret;
	struct data *data = data_alloc(DATA_TYPE_RESULT, 1);
	double mflops;
	char *out;

	ret = subproc_call_get_stdout(d->bin, d->args, &out);

	if (ret) {
		data_put(data);
		linpack_uninstall(plug);
		return ret;
	}

	mflops = atof(out) / 1000;
	data_add_double(data, mflops);

	plugin_add_results(plug, data);

	return ret;
}

static const struct header *linpack_data_hdr(struct plugin *plug)
{
	static const struct header hdr[] = {
		{
			.name = "MFLOPS",
			.description = "MFLOPS calculated by linpack benchmark.",
			.unit = "s",
			.data_type = DATA_MORE_IS_BETTER,
		}, {
			/* Sentinel */
		}
	};

	return hdr;
}

const struct plugin_id plugin_linpack = {
	.name = "linpack",
	.description = "Linpack benchmark",
	.install = linpack_install,
	.uninstall = linpack_uninstall,
	.run = linpack_run,
	.data_hdr = linpack_data_hdr,
	.versions = plugin_linpack_versions,
};
