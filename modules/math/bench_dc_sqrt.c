#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <cbench/option.h>
#include <cbench/plugin_id_helper.h>

static struct requirement plugin_dc_sqrt_requirements[] = {
	{
		.name = "dc",
		.description = "dc calculator",
	}, {
		/* Sentinel */
	}
};

static struct version plugin_dc_sqrt_versions[] = {
	{
		.version = "0.1",
		.nr_independent_values = 1,
		.requirements = plugin_dc_sqrt_requirements,
	}, {
		/* Sentinel */
	}
};

static char dc_sqrt_bin[] = "dc";
static char dc_sqrt_arg_f[] = "-f";

struct dc_sqrt_data {
	char *dc_file;
	char *args[4];
};

static int dc_sqrt_install(struct plugin *plug)
{
	struct dc_sqrt_data *d = malloc(sizeof(*d));
	const char *bin_path = plugin_get_bin_path(plug);
	char *dc_file;

	if (!d)
		return -1;

	dc_file = malloc(strlen(bin_path) + 24);
	if (!dc_file) {
		free(d);
		return -1;
	}

	sprintf(dc_file, "%s/sqrt.dc", bin_path);
	d->dc_file = dc_file;
	d->args[2] = dc_file;

	d->args[0] = dc_sqrt_bin;
	d->args[1] = dc_sqrt_arg_f;
	d->args[3] = NULL;

	plugin_set_data(plug, d);

	return 0;
}

static int dc_sqrt_uninstall(struct plugin *plug)
{
	struct dc_sqrt_data *d = plugin_get_data(plug);

	if (!d)
		return 0;

	free(d->dc_file);
	free(d);

	plugin_set_data(plug, NULL);

	return 0;
}

static int dc_sqrt_run(struct plugin *plug)
{
	struct dc_sqrt_data *d = plugin_get_data(plug);
	int ret;
	struct data *data = data_alloc(DATA_TYPE_RESULT, 1);
	struct timespec start;
	struct timespec end;
	double runtime;

	clock_gettime(CLOCK_MONOTONIC, &start);

	ret = subproc_call(dc_sqrt_bin, d->args);

	if (ret) {
		data_put(data);
		dc_sqrt_uninstall(plug);
		return ret;
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	runtime = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	data_add_double(data, runtime);

	plugin_add_results(plug, data);

	return ret;
}

static const struct header *dc_sqrt_data_hdr(struct plugin *plug)
{
	static const struct header hdr[] = {
		{
			.name = "runtime",
			.description = "Runtime of dc_sqrt benchmark.",
			.unit = "s",
			.data_type = DATA_LESS_IS_BETTER,
		}, {
			/* Sentinel */
		}
	};

	return hdr;
}


