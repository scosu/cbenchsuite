#include <stdio.h>
#include <stdlib.h>

#include <cbench/option.h>
#include <cbench/plugin_id_helper.h>

struct header plugin_hackbench_defaults[] = {
	OPTION_BOOL("pipe", NULL, NULL, 0),
	OPTION_BOOL("process", "Use processes instead of threads.", NULL, 0),
	OPTION_INT32("groups", "Number of groups used.", NULL, 10),
	OPTION_INT32("loops", "Number of loops executed.", NULL, 10000),
	OPTION_SENTINEL
};

static struct version plugin_hackbench_versions[] = {
	{
		.version = "0.1",
		.default_options = plugin_hackbench_defaults,
		.nr_independent_values = 1,
	}, {
		/* Sentinel */
	}
};

static char hackbench_pipe[] = "-pipe";
static char hackbench_process[] = "process";
static char hackbench_thread[] = "thread";

struct hackbench_data {
	char *hb_bin;
	char loops[32];
	char groups[32];
	char *args[6];

	char *result;
};

static int hackbench_install(struct plugin *plug)
{
	struct hackbench_data *d = malloc(sizeof(*d));
	const struct header *opts = plugin_get_options(plug);
	const char *bin_path = plugin_get_bin_path(plug);
	char *hb_bin;
	int argi = 1;

	if (!d)
		return -1;

	hb_bin = malloc(strlen(bin_path) + 24);
	if (!hb_bin) {
		free(d);
		return -1;
	}

	sprintf(hb_bin, "%s/hackbench", bin_path);
	d->hb_bin = hb_bin;
	d->args[0] = hb_bin;

	if (option_get_int32(opts, "pipe")) {
		d->args[1] = hackbench_pipe;
		++argi;
	}

	sprintf(d->groups, "%d", option_get_int32(opts, "groups"));
	d->args[argi] = d->groups;
	++argi;

	if (option_get_int32(opts, "process"))
		d->args[argi] = hackbench_process;
	else
		d->args[argi] = hackbench_thread;
	++argi;

	sprintf(d->loops, "%d", option_get_int32(opts, "loops"));
	d->args[argi] = d->loops;
	++argi;

	d->args[argi] = NULL;

	plugin_set_data(plug, d);

	return 0;
}

static int hackbench_uninstall(struct plugin *plug)
{
	struct hackbench_data *d = plugin_get_data(plug);

	if (!d)
		return 0;

	if (d->result)
		free(d->result);

	free(d->hb_bin);
	free(d);

	plugin_set_data(plug, NULL);

	return 0;
}

static int hackbench_parse_results(struct plugin *plug)
{
	struct hackbench_data *d = plugin_get_data(plug);
	struct data *data = data_alloc(DATA_TYPE_RESULT, 1);
	double res;

	res = atof(d->result);
	data_add_double(data, res);

	plugin_add_results(plug, data);

	free(d->result);
	d->result = NULL;
	return 0;
}

static int hackbench_run(struct plugin *plug)
{
	struct hackbench_data *d = plugin_get_data(plug);
	int ret;

	ret = subproc_call_get_stdout(d->hb_bin, d->args, &d->result);

	if (ret) {
		hackbench_uninstall(plug);
	}
	return ret;
}

static const struct header *hackbench_data_hdr(struct plugin *plug)
{
	static const struct header hdr[] = {
		{
			.name = "runtime",
			.description = "Runtime of hackbench benchmark.",
			.unit = "s",
			.data_type = DATA_LESS_IS_BETTER,
		}, {
			/* Sentinel */
		}
	};

	return hdr;
}


