/*
 *
 * sched-pipe.c
 *
 * pipe: Benchmark for pipe()
 *
 * Based on pipe-test-1m.c by Ingo Molnar <mingo@redhat.com>
 *  http://people.redhat.com/mingo/cfs-scheduler/tools/pipe-test-1m.c
 * Ported to perf by Hitoshi Mitake <mitake@dcl.info.waseda.ac.jp>
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <linux/unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>

struct header plugin_sched_pipe_defaults[] = {
	OPTION_INT64("loops", "Number of loops executed.", NULL, 5000000),
	OPTION_SENTINEL
};

static struct version plugin_sched_pipe_versions[] = {
	{
		.version = "0.1",
		.default_options = plugin_sched_pipe_defaults,
		.nr_independent_values = 1,
	}, {
		/* Sentinel */
	}
};

static const struct header *sched_pipe_data_hdr(struct plugin *plug)
{
	static const struct header hdr[] = {
		{
			.name = "runtime",
			.description = "Runtime of sched-pipe benchmark.",
			.unit = "s",
			.data_type = DATA_LESS_IS_BETTER,
		}, {
			/* Sentinel */
		}
	};

	return hdr;
}

int sched_pipe_run(struct plugin *plug)
{
	int pipe_1[2], pipe_2[2];
	int m = 0, i;
	struct timeval start, stop, diff;
	const struct header *opts = plugin_get_options(plug);
	uint64_t loops = option_get_int64(opts, "loops");
	double result;
	int ret;
	int wait_stat;
	pid_t pid, retpid;
	struct data *d;

	ret = pipe(pipe_1);
	if (ret)
		return -1;

	ret = pipe(pipe_2);
	if (ret)
		return -1;

	pid = fork();
	if (pid < 0)
		return -1;

	gettimeofday(&start, NULL);

	if (!pid) {
		for (i = 0; i < loops; i++) {
			ret = read(pipe_1[0], &m, sizeof(int));
			ret = write(pipe_2[1], &m, sizeof(int));
		}
	} else {
		for (i = 0; i < loops; i++) {
			ret = write(pipe_1[1], &m, sizeof(int));
			ret = read(pipe_2[0], &m, sizeof(int));
		}
	}

	gettimeofday(&stop, NULL);
	timersub(&stop, &start, &diff);

	if (pid) {
		retpid = waitpid(pid, &wait_stat, 0);
		if (retpid != pid || !WIFEXITED(wait_stat))
			return -1;
	} else {
		exit(0);
	}

	d = data_alloc(DATA_TYPE_RESULT, 1);

	if (!d)
		return -1;

	result = diff.tv_sec + diff.tv_usec / 1000000.0;

	data_add_double(d, result);

	plugin_add_results(plug, d);

	return 0;
}
