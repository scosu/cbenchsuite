#include <stdio.h>
#include <stdlib.h>

#include <cbench/option.h>
#include <cbench/plugin_id_helper.h>

struct header plugin_p7zip_bench_defaults[] = {
	OPTION_INT32("threads", NULL, NULL, 16),
	OPTION_INT32("dictsize", "Size of the dictionary used. 2^dictsize bytes.", NULL, 25),
	OPTION_SENTINEL
};

static struct requirement p7zip_requirements[] = {
	{
		.name = "p7zip",
	}, {
		/* Sentinel */
	}
};

static struct comp_version p7zip_comp_versions[] = {
	{
		.name = "p7zip",
		.version = NULL,
	}, {
		/* Sentinel */
	}
};

static struct version plugin_p7zip_bench_versions[] = {
	{
		.version = "0.1",
		.default_options = plugin_p7zip_bench_defaults,
		.requirements = p7zip_requirements,
		.nr_independent_values = 2,
		.comp_versions = p7zip_comp_versions,
	}, {
		/* Sentinel */
	}
};

static char p7zip_bin[] = "7z";
static char p7zip_arg_bench[] = "b";
static char p7zip_arg_loops[] = "1";

struct p7zip_bench_data {
	char threads[32];
	char dict[32];
	char *args[6];

	char *result;
};

static int p7zip_init_7z()
{
	char *args[] = {"7z", NULL};
	char *ver_info;
	int ret;
	char *ver_start;
	char *ver_end;
	char *version;

	ret = subproc_call_get_stdout("7z", args, &ver_info);

	if (ret)
		return -1;

	ver_start = strstr(ver_info, "] ");
	if (!ver_start)
		goto error;

	ver_start += 2;

	ver_end = strstr(ver_start, " ");
	if (!ver_end)
		goto error;
	*ver_end = '\0';

	version = malloc(strlen(ver_start) + 1);
	if (!version)
		goto error;

	strcpy(version, ver_start);
	p7zip_comp_versions[0].version = version;

	p7zip_requirements[0].found = 1;

	free(ver_info);
	return 0;
error:
	free(ver_info);
	return -1;
}

static void p7zip_exit_7z()
{
	if (p7zip_comp_versions[0].version) {
		free(p7zip_comp_versions[0].version);
		p7zip_comp_versions[0].version = NULL;
	}
}

static int p7zip_bench_install(struct plugin *plug)
{
	struct p7zip_bench_data *d = malloc(sizeof(*d));
	const struct header *opts = plugin_get_options(plug);
	int dictsize;

	if (!d)
		return -1;

	d->args[0] = p7zip_bin;
	d->args[1] = p7zip_arg_bench;
	d->args[2] = p7zip_arg_loops;

	sprintf(d->threads, "-mmt=%d", option_get_int32(opts, "threads"));
	d->args[3] = d->threads;

	dictsize = option_get_int32(opts, "dictsize");
	if (dictsize < 18) {
		printf("Error: dictionary is too small\n");
		free(d);
		return -1;
	}

	sprintf(d->dict, "-md=%d", dictsize);
	d->args[4] = d->dict;

	d->args[5] = NULL;

	plugin_set_data(plug, d);

	return 0;
}

static int p7zip_bench_uninstall(struct plugin *plug)
{
	struct p7zip_bench_data *d = plugin_get_data(plug);

	if (!d)
		return 0;

	if (d->result)
		free(d->result);

	free(d);

	plugin_set_data(plug, NULL);

	return 0;
}

static int p7zip_bench_parse_results(struct plugin *plug)
{
	struct p7zip_bench_data *d = plugin_get_data(plug);
	struct data *data = data_alloc(DATA_TYPE_RESULT, 2);
	const struct header *opts = plugin_get_options(plug);
	char filter[32];
	int dictsize = option_get_int32(opts, "dictsize");
	char *line;

	sprintf(filter, "\n%d:", dictsize);
	line = strstr(d->result, filter);
	if (line == NULL) {
		fputs(d->result, stderr);
		fprintf(stderr, "Error: Can't find the dictionary size in the results: %s\n",
				filter);
		goto error;
	}

	line += strlen(filter);

	data_add_int32(data, atoi(line));

	line = strstr(line, "|");
	if (line == NULL) {
		fputs(d->result, stderr);
		fprintf(stderr, "Error: Can't find the decompress value\n");
		goto error;
	}
	++line;

	data_add_int32(data, atoi(line));

	plugin_add_results(plug, data);

	free(d->result);
	d->result = NULL;
	return 0;
error:
	data_put(data);
	p7zip_bench_uninstall(plug);
	return -1;
}

static int p7zip_bench_run(struct plugin *plug)
{
	struct p7zip_bench_data *d = plugin_get_data(plug);
	int ret;

	ret = subproc_call_get_stdout(p7zip_bin, d->args, &d->result);

	if (ret) {
		p7zip_bench_uninstall(plug);
	}
	return ret;
}

static const struct header *p7zip_bench_data_hdr(struct plugin *plug)
{
	static const struct header hdr[] = {
		{
			.name = "compress_speed",
			.description = "Compression speed of p7zip.",
			.unit = "KB/s",
			.data_type = DATA_MORE_IS_BETTER,
		}, {
			.name = "decompress_speed",
			.description = "Decompression speed of p7zip.",
			.unit = "KB/s",
			.data_type = DATA_MORE_IS_BETTER,
		}, {
			/* Sentinel */
		}
	};

	return hdr;
}


