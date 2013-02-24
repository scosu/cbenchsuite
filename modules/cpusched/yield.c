/*
 * Cbench - A C benchmarking suite for Linux benchmarking.
 * Copyright (C) 2013  Markus Pargmann <mpargmann@allfex.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sched.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <cbench/option.h>
#include <cbench/plugin.h>
#include <cbench/plugin_id_helper.h>
#include <cbench/version.h>

struct yield_bench_thread_data {
	pthread_t thread;
	pthread_barrier_t *barrier;
	int shutdown;
	uint64_t loops;
};

struct yield_bench_data {
	pthread_barrier_t barrier;
	struct yield_bench_thread_data *threads;
	int nr_threads;
	int seconds;
	struct timespec start;
	struct timespec end;
};

static void *yield_bench_thread(void *data)
{
	struct yield_bench_thread_data *d = data;
	uint64_t loops = 0;
	d->shutdown = 0;
	pthread_barrier_wait(d->barrier);
	while (!d->shutdown) {
		sched_yield();
		++loops;
	}

	d->loops = loops;
	return NULL;
}

static int yield_bench_init(struct plugin *plug)
{
	struct yield_bench_data *data = malloc(sizeof(*data));
	const struct option *options = plugin_get_options(plug);
	int i;
	int ret;
	if (!data)
		return -1;
	data->nr_threads = option_get_int32(options, "threads");
	if (data->nr_threads <= 0) {
		free(data);
		return -1;
	}

	data->seconds = option_get_int32(options, "seconds");
	if (data->seconds <= 0) {
		free(data);
		return -1;
	}

	data->threads = malloc(sizeof(struct yield_bench_thread_data) * data->nr_threads);
	if (!data->threads) {
		free(data);
		return -1;
	}

	pthread_barrier_init(&data->barrier, NULL, data->nr_threads + 1);
	for (i = 0; i != data->nr_threads; ++i) {
		data->threads[i].barrier = &data->barrier;
		ret = pthread_create(&data->threads[i].thread, NULL,
				yield_bench_thread, &data->threads[i]);
		if (ret)
			return -1;
	}

	plugin_set_data(plug, data);

	return 0;
}

static int yield_bench_run(struct plugin *plug)
{
	struct yield_bench_data *data = plugin_get_data(plug);
	int i;

	clock_gettime(CLOCK_MONOTONIC_RAW, &data->start);
	pthread_barrier_wait(&data->barrier);
	sleep(data->seconds);

	for (i = 0; i != data->nr_threads; ++i) {
		data->threads[i].shutdown = 1;
	}

	for (i = 0; i != data->nr_threads; ++i) {
		pthread_join(data->threads[i].thread, NULL);
	}
	clock_gettime(CLOCK_MONOTONIC_RAW, &data->end);
	return 0;
}

static int yield_bench_parse_results(struct plugin *plug)
{
	struct yield_bench_data *data = plugin_get_data(plug);
	uint64_t abs_time;
	uint64_t should_time;
	uint64_t loops = 0;
	struct data *result;
	int i;

	abs_time = (data->end.tv_sec - data->start.tv_sec) * 1000000
		+ (data->end.tv_nsec - data->start.tv_nsec) / 1000;
	should_time = data->seconds * 1000000;

	for (i = 0; i != data->nr_threads; ++i) {
		loops += data->threads[i].loops;
	}

	loops = (loops * should_time) / abs_time;
	result = data_alloc(DATA_TYPE_RESULT, 1);

	data_add_int64(result, loops);

	plugin_add_results(plug, result);
	return 0;
}

static int yield_bench_exit(struct plugin *plug)
{
	struct yield_bench_data *data = plugin_get_data(plug);

	free(data->threads);
	free(data);

	plugin_set_data(plug, NULL);
	return 0;
}

static const struct value *yield_bench_data_hdr(struct plugin *plug)
{
	static const struct value hdr[] = {
		{ .v_str = "Iterations" },
		VALUE_STATIC_SENTINEL
	};

	return hdr;
}

static struct option yield_bench_options[] = {
	OPTION_INT32("threads", 16),
	OPTION_INT32("seconds", 30),
	OPTION_SENTINEL
};

static struct version yield_bench_versions[] = {
	{
		.version = "1.0",
		.nr_independent_values = 1,
		.default_options = yield_bench_options,
	}, {
		/* Sentinel */
	}
};

#define yield_bench_plugin_id { 					\
	.name = "yield-bench", 						\
	.description = "This benchmark stresses the cpu kernel scheduler by"\
			" repeating sched_yield calls for a given time.",\
	.versions = yield_bench_versions, 				\
									\
	.init = yield_bench_init, 					\
	.run = yield_bench_run, 					\
	.exit = yield_bench_exit, 					\
	.parse_results = yield_bench_parse_results, 			\
	.data_hdr = yield_bench_data_hdr, 				\
},

