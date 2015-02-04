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
#include <stdio.h>
#include <unistd.h>

#include <cbench/data.h>
#include <cbench/requirement.h>
#include <cbench/version.h>
#include <cbench/plugin.h>

static const char monitor_schedstat_path[] = "/proc/schedstat";

static struct requirement plugin_monitor_schedstat_requirements[] = {
	{
		.name = monitor_schedstat_path,
		.description = "This plugin uses /proc/schedstat to monitor the systyem.",
		.found = 0,
	},
	{ }
};

static struct version plugin_monitor_schedstat_versions[] = {
	{
		.version = "0.1",
		.requirements = plugin_monitor_schedstat_requirements,
	}, {
		/* Sentinel */
	}
};

struct monitor_schedstat {
	uint64_t yields;
	uint64_t schedules;
	uint64_t schedules_idle;
	uint64_t ttwus;
};

struct monitor_schedstat_data {
	char *buf;
	size_t buf_len;
	int first;

	double start_time;
	struct monitor_schedstat initial;
};

static int monitor_schedstat_install(struct plugin *plug)
{
	struct monitor_schedstat_data *d = malloc(sizeof(*d));

	if (!d)
		return -1;

	d->buf = NULL;

	plugin_set_data(plug, d);

	return 0;
}

static int monitor_schedstat_uninstall(struct plugin *plug)
{
	struct monitor_schedstat_data *d = plugin_get_data(plug);

	if (d->buf) {
		free(d->buf);
		d->buf = NULL;
	}

	free(d);

	plugin_set_data(plug, NULL);
	return 0;
}

static int monitor_schedstat_init(struct plugin *plug)
{
	struct monitor_schedstat_data *d = plugin_get_data(plug);

	d->first = 1;
	return 0;
}

static int monitor_schedstat_get(struct plugin *plug, struct monitor_schedstat *stat)
{
	FILE *f;
	int found = 0;
	struct monitor_schedstat_data *d = plugin_get_data(plug);
	struct monitor_schedstat stat_rd;

	memset(stat, 0, sizeof(*stat));

	f = fopen(monitor_schedstat_path, "r");
	if (!f) {
		printf("Error: Could not open file %s\n", monitor_schedstat_path);
		return -1;
	}

	while (0 < getline(&d->buf, &d->buf_len, f)) {
		int ret;
		uint64_t discard;

		ret = sscanf(d->buf, "cpu%" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " ",
				&discard,
				&stat_rd.yields, &discard, &stat_rd.schedules,
				&stat_rd.schedules_idle, &stat_rd.ttwus);
		if (ret != 6) {
			continue;
		}

		found = 1;
		stat->yields += stat_rd.yields;
		stat->schedules += stat_rd.schedules;
		stat->schedules_idle += stat_rd.schedules_idle;
		stat->ttwus += stat_rd.ttwus;
	}

	fclose(f);

	if (!found) {
		printf("Failed to find some data in %s\n", monitor_schedstat_path);
		monitor_schedstat_uninstall(plug);
		return -1;
	}

	return 0;
}

static int monitor_schedstat_mon(struct plugin *plug)
{
	struct monitor_schedstat_data *d = plugin_get_data(plug);
	struct monitor_schedstat schedstat;
	struct data *dat;
	int ret;
	struct timespec time;
	double now;

	clock_gettime(CLOCK_MONOTONIC, &time);

	ret = monitor_schedstat_get(plug, &schedstat);
	if (ret)
		return ret;

	dat = data_alloc(DATA_TYPE_MONITOR, 5);
	if (!dat)
		return -1;

	now = time.tv_sec + time.tv_nsec / 1000000000.0;

	if (d->first) {
		d->initial = schedstat;
		d->start_time = now;
		d->first = 0;
	}

	now -= d->start_time;
	schedstat.yields -= d->initial.yields;
	schedstat.schedules -= d->initial.schedules;
	schedstat.schedules_idle -= d->initial.schedules_idle;
	schedstat.ttwus -= d->initial.ttwus;

	data_add_double(dat, now);
	data_add_int64(dat, schedstat.yields);
	data_add_int64(dat, schedstat.schedules);
	data_add_int64(dat, schedstat.schedules_idle);
	data_add_int64(dat, schedstat.ttwus);

	plugin_add_results(plug, dat);

	return 0;
}

static const struct header *monitor_schedstat_data_hdr(struct plugin *plug)
{
	static const struct header hdr[] = {
		{
			.name = "time",
			.unit = "s",
			.description = "Time of this measurement.",
		}, {
			.name = "sched_yield calls",
			.description = "Number of sched_yield{} calls",
		}, {
			.name = "schedule calls",
			.description = "Number of schedule() calls",
		}, {
			.name = "schedule idle calls",
			.description = "Number of schedule() calls that left the processor idle.",
		}, {
			.name = "try_to_wake_up calls",
			.description = "Number of try_to_wake_up() calls",
		}, {
			/* Sentinel */
		}
	};

	return hdr;
}

static int monitor_schedstat_mod_init(struct module *mod,
				      const struct plugin_id *plug)
{
	if (!access(monitor_schedstat_path, F_OK | R_OK))
		plugin_monitor_schedstat_requirements[0].found = 1;

	return 0;
}

const struct plugin_id plugin_schedstat = {
	.name = "monitor-schedstat",
	.description = "Monitor plugin to keep track of different values shown in /proc/schedstat.",
	.module_init = monitor_schedstat_mod_init,
	.install = monitor_schedstat_install,
	.uninstall = monitor_schedstat_uninstall,
	.init = monitor_schedstat_init,
	.monitor = monitor_schedstat_mon,
	.versions = plugin_monitor_schedstat_versions,
	.data_hdr = monitor_schedstat_data_hdr,
};
