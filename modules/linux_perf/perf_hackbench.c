#include <stdio.h>
#include <stdlib.h>

#include <cbench/option.h>
#include <cbench/plugin_id_helper.h>
#include <cbench/requirement.h>
#include <cbench/version.h>
#include <cbench/exec_helper.h>

struct header plugin_hackbench_defaults[] = {
	OPTION_BOOL("pipe", "Use a pipe instead of Unix domain sockets", NULL, 0),
	OPTION_BOOL("process", "Use processes instead of threads.", NULL, 0),
	OPTION_INT32("groups", "Number of groups used.", NULL, 10),
	OPTION_INT32("loops", "Number of loops executed.", NULL, 10000),
	OPTION_INT32("size", "Number of bytes transfered in each message.", NULL, 100),
	OPTION_INT32("fds", "Number of file descriptor pair opened.", NULL, 20),
	OPTION_SENTINEL
};

static struct requirement hackbench_requirements[] = {
	{
		.name = "hackbench",
	}, {
		/* Sentinel */
	}
};

static struct version plugin_hackbench_versions[] = {
	{
		.version = "0.2",
		.default_options = plugin_hackbench_defaults,
		.requirements = hackbench_requirements,
		.nr_independent_values = 1,
	}, {
		/* Sentinel */
	}
};

static char hackbench_pipe[] = "-pipe";
static char hackbench_process[] = "--process";
static char hackbench_thread[] = "--threads";
static char hackbench_groups[] = "--groups";
static char hackbench_loops[] = "--loops";
static char hackbench_size[] = "--datasize";
static char hackbench_fds[] = "--fds";

struct hackbench_data {
	char *hb_bin;
	char groups[32];
	char loops[32];
	char size[32];
	char fds[32];
	char *args[12];

	char *result;
};

static int hackbench_init(struct module *mod, const struct plugin_id *plug)
{
	char *args[] = {"hackbench", "-h", NULL};
	int ret;

	ret = subproc_call("hackbench", args);
	if (WIFEXITED(ret) && WEXITSTATUS(ret) == 1)
		hackbench_requirements[0].found = 1;

	return 0;
}

static int hackbench_install(struct plugin *plug)
{
	struct hackbench_data *d = malloc(sizeof(*d));
	const struct header *opts = plugin_get_options(plug);
	char *hb_bin;
	int argi = 1;

	if (!d)
		return -1;

	hb_bin = strdup("/usr/bin/hackbench");
	if (!hb_bin) {
		free(d);
		return -1;
	}

	d->hb_bin = hb_bin;
	d->args[0] = hb_bin;

	if (option_get_int32(opts, "pipe")) {
		d->args[1] = hackbench_pipe;
		++argi;
	}

	d->args[argi] = hackbench_groups;
	++argi;
	sprintf(d->groups, "%d", option_get_int32(opts, "groups"));
	d->args[argi] = d->groups;
	++argi;

	if (option_get_int32(opts, "process"))
		d->args[argi] = hackbench_process;
	else
		d->args[argi] = hackbench_thread;
	++argi;

	d->args[argi] = hackbench_loops;
	++argi;
	sprintf(d->loops, "%d", option_get_int32(opts, "loops"));
	d->args[argi] = d->loops;
	++argi;

	d->args[argi] = hackbench_size;
	++argi;
	sprintf(d->size, "%d", option_get_int32(opts, "size"));
	d->args[argi] = d->size;
	++argi;

	d->args[argi] = hackbench_fds;
	++argi;
	sprintf(d->fds, "%d", option_get_int32(opts, "fds"));
	d->args[argi] = d->fds;
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
	char *c;

	c = strrchr(d->result, ':');
	if (!c)
		return -1;

	res = atof(c+2);
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

const struct plugin_id plugin_hackbench = {
	.name = "hackbench",
	.description = "Benchmark that spawns a number of groups that internally send/receive packets. Also known within the linux kernel perf tool as sched-messaging.",
	.module_init = hackbench_init,
	.install = hackbench_install,
	.uninstall = hackbench_uninstall,
	.parse_results = hackbench_parse_results,
	.run = hackbench_run,
	.data_hdr = hackbench_data_hdr,
	.versions = plugin_hackbench_versions,
};
