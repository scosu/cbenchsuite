#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <cbench/option.h>
#include <cbench/plugin_id_helper.h>

static struct header plugin_whet_options[] = {
	OPTION_INT64("loops", "Number of loops to execute", NULL, 2000000),
	OPTION_SENTINEL
};

static struct version plugin_whet_versions[] = {
	{
		.version = "0.1",
		.nr_independent_values = 1,
		.default_options = plugin_whet_options,
	}, {
		/* Sentinel */
	}
};

struct whet_data {
	char *bin;
	char iterations[32];
	char *args[4];
};

static int whet_install(struct plugin *plug)
{
	struct whet_data *d = malloc(sizeof(*d));
	const char *bin_path = plugin_get_bin_path(plug);
	const struct header *opts = plugin_get_options(plug);

	if (!d)
		return -1;

	d->bin = malloc(strlen(bin_path) + 16);
	if (!d->bin)
		return -1;

	sprintf(d->bin, "%s/whetstone", bin_path);

	sprintf(d->iterations, "%d", option_get_int32(opts, "loops"));

	d->args[0] = d->bin;
	d->args[1] = d->iterations;
	d->args[2] = NULL;

	plugin_set_data(plug, d);

	return 0;
}

static int whet_uninstall(struct plugin *plug)
{
	struct whet_data *d = plugin_get_data(plug);

	if (!d)
		return 0;

	free(d->bin);
	free(d);

	plugin_set_data(plug, NULL);

	return 0;
}

static int whet_run(struct plugin *plug)
{
	struct whet_data *d = plugin_get_data(plug);
	int ret;
	struct data *data = data_alloc(DATA_TYPE_RESULT, 1);
	struct timespec start;
	struct timespec end;
	double runtime;

	clock_gettime(CLOCK_MONOTONIC, &start);

	ret = subproc_call(d->bin, d->args);

	if (ret) {
		data_put(data);
		whet_uninstall(plug);
		return ret;
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	runtime = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	data_add_double(data, runtime);

	plugin_add_results(plug, data);

	return ret;
}

static const struct header *whet_data_hdr(struct plugin *plug)
{
	static const struct header hdr[] = {
		{
			.name = "runtime",
			.description = "Runtime of whetstone benchmark.",
			.unit = "s",
			.data_type = DATA_LESS_IS_BETTER,
		}, {
			/* Sentinel */
		}
	};

	return hdr;
}


