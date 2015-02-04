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

#include <inttypes.h>
#include <stdint.h>
#include <time.h>

#include <cbench/data.h>
#include <cbench/plugin.h>
#include <cbench/version.h>

static struct version plugin_monitor_latency_versions[] = {
	{
		.version = "0.1",
	}, {
		/* Sentinel */
	}
};

static const struct timespec monitor_latency_sleep = {
	.tv_sec = 0,
	.tv_nsec = 500000000,
};

struct monitor_latency_data {
	int stop;
};

static int monitor_latency_install(struct plugin *plug)
{
	struct monitor_latency_data *d = malloc(sizeof(*d));

	if (!d)
		return -1;

	plugin_set_data(plug, d);

	return 0;
}

static int monitor_latency_uninstall(struct plugin *plug)
{
	struct monitor_latency_data *d = plugin_get_data(plug);

	free(d);

	plugin_set_data(plug, NULL);
	return 0;
}

static int monitor_latency_init(struct plugin *plug)
{
	struct monitor_latency_data *d = plugin_get_data(plug);

	d->stop = 0;

	return 0;
}

static int monitor_latency_run(struct plugin *plug)
{
	struct monitor_latency_data *d = plugin_get_data(plug);
	struct timespec now;
	double now_s;
	double start_time;
	double last_time;

	clock_gettime(CLOCK_MONOTONIC, &now);
	last_time = now.tv_sec + now.tv_nsec / 1000000000.0;
	start_time = last_time;

	while (!d->stop) {
		struct data *dat;

		nanosleep(&monitor_latency_sleep, NULL);
		clock_gettime(CLOCK_MONOTONIC, &now);

		dat = data_alloc(DATA_TYPE_MONITOR, 2);
		if (!dat)
			return -1;

		now_s = now.tv_sec + now.tv_nsec / 1000000000.0;
		data_add_double(dat, now_s - start_time);
		data_add_double(dat, now_s - last_time - 0.5);
		plugin_add_results(plug, dat);

		clock_gettime(CLOCK_MONOTONIC, &now);
		last_time = now.tv_sec + now.tv_nsec / 1000000000.0;
	}

	return 0;
}

static void monitor_latency_stop(struct plugin *plug)
{
	struct monitor_latency_data *d = plugin_get_data(plug);
	d->stop = 1;
}

static const struct header *monitor_latency_data_hdr(struct plugin *plug)
{
	static const struct header hdr[] = {
		{
			.name = "time",
			.unit = "s",
			.description = "Time of this measurement.",
		}, {
			.name = "latency",
			.unit = "s",
			.description = "Latency for scheduling a sleeper thread onto the CPU again.",
		}, {
			/* Sentinel */
		}
	};

	return hdr;
}

const struct plugin_id plugin_latency_monitor = {
	.name = "latency-monitor",
	.description = "A monitor that measures the latency to wakeup a sleeping task every 0.5 seconds.",
	.versions = plugin_monitor_latency_versions,
	.init = monitor_latency_init,
	.run = monitor_latency_run,
	.stop = monitor_latency_stop,
	.data_hdr = monitor_latency_data_hdr,
	.install = monitor_latency_install,
	.uninstall = monitor_latency_uninstall,
};
