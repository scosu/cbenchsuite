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

static const char monitor_stats_path[] = "/proc/stat";

static struct requirement plugin_monitor_stats_requirements[] = {
	{
		.name = monitor_stats_path,
		.description = "This plugin uses /proc/stat to monitor the systyem.",
		.found = 0,
	},
	{ }
};

static struct version plugin_monitor_stats_versions[] = {
	{
		.version = "0.1",
		.requirements = plugin_monitor_stats_requirements,
	}, {
		/* Sentinel */
	}
};

struct monitor_stats {
	uint64_t cpu_user;
	uint64_t cpu_nice;
	uint64_t cpu_system;
	uint64_t cpu_idle;
	uint64_t cpu_iowait;
	uint64_t cpu_irq;
	uint64_t cpu_softirq;
	uint64_t cpu_steal;
	uint64_t cpu_guest;
	uint64_t cpu_guest_nice;

	uint64_t intr_total;

	uint64_t ctxt;
	uint64_t processes;
	uint64_t procs_running;
	uint64_t procs_blocked;
};

struct monitor_stat_data {
	char *buf;
	size_t buf_len;
	int first;

	double start_time;
	struct monitor_stats initial;
};

static int monitor_stat_install(struct plugin *plug)
{
	struct monitor_stat_data *d = malloc(sizeof(*d));

	if (!d)
		return -1;

	d->buf = NULL;

	plugin_set_data(plug, d);

	return 0;
}

static int monitor_stat_uninstall(struct plugin *plug)
{
	struct monitor_stat_data *d = plugin_get_data(plug);

	if (d->buf) {
		free(d->buf);
		d->buf = NULL;
	}

	free(d);

	plugin_set_data(plug, NULL);
	return 0;
}

static int monitor_stat_init(struct plugin *plug)
{
	struct monitor_stat_data *d = plugin_get_data(plug);

	d->first = 1;
	return 0;
}

static int monitor_stat_get(struct plugin *plug, struct monitor_stats *stat)
{
	FILE *f;
	int found = 0;
	struct monitor_stat_data *d = plugin_get_data(plug);

	f = fopen(monitor_stats_path, "r");
	if (!f) {
		printf("Error: Could not open file %s\n", monitor_stats_path);
		return -1;
	}

	while (0 < getline(&d->buf, &d->buf_len, f)) {
		int ret;

		if (!strncmp(d->buf, "cpu ", 4)) {
			ret = sscanf(d->buf, "cpu %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 "\n",
					&stat->cpu_user, &stat->cpu_nice, &stat->cpu_system,
					&stat->cpu_idle, &stat->cpu_iowait,
					&stat->cpu_irq, &stat->cpu_softirq,
					&stat->cpu_steal, &stat->cpu_guest,
					&stat->cpu_guest_nice);
			if (ret == 10) {
				found |= 0x1;
				continue;
			}
		}

		ret = sscanf(d->buf, "intr %" PRIu64 " ", &stat->intr_total);
		if (ret == 1) {
			found |= 0x2;
			continue;
		}

		ret = sscanf(d->buf, "ctxt %" PRIu64 " ", &stat->ctxt);
		if (ret == 1) {
			found |= 0x4;
			continue;
		}

		ret = sscanf(d->buf, "processes %" PRIu64 " ", &stat->processes);
		if (ret == 1) {
			found |= 0x8;
			continue;
		}

		ret = sscanf(d->buf, "procs_running %" PRIu64 " ", &stat->procs_running);
		if (ret == 1) {
			found |= 0x10;
			continue;
		}

		ret = sscanf(d->buf, "procs_blocked %" PRIu64 " ", &stat->procs_blocked);
		if (ret == 1) {
			found |= 0x20;
			continue;
		}

	}

	fclose(f);

	if (found != 0x3f) {
		printf("Failed to find some data in %s\n", monitor_stats_path);
		monitor_stat_uninstall(plug);
		return -1;
	}

	return 0;
}

static int monitor_stat_mon(struct plugin *plug)
{
	struct monitor_stat_data *d = plugin_get_data(plug);
	struct monitor_stats stats;
	struct data *dat;
	int ret;
	struct timespec time;
	double now;

	clock_gettime(CLOCK_MONOTONIC, &time);

	ret = monitor_stat_get(plug, &stats);
	if (ret)
		return ret;

	dat = data_alloc(DATA_TYPE_MONITOR, 16);
	if (!dat)
		return -1;

	now = time.tv_sec + time.tv_nsec / 1000000000.0;

	if (d->first) {
		d->initial = stats;
		d->start_time = now;
		d->first = 0;
	}

	now -= d->start_time;
	stats.cpu_user -= d->initial.cpu_user;
	stats.cpu_nice -= d->initial.cpu_nice;
	stats.cpu_system -= d->initial.cpu_system;
	stats.cpu_idle -= d->initial.cpu_idle;
	stats.cpu_iowait -= d->initial.cpu_iowait;
	stats.cpu_irq -= d->initial.cpu_irq;
	stats.cpu_softirq -= d->initial.cpu_softirq;
	stats.cpu_steal -= d->initial.cpu_steal;
	stats.cpu_guest -= d->initial.cpu_guest;
	stats.cpu_guest_nice -= d->initial.cpu_guest_nice;
	stats.ctxt -= d->initial.ctxt;
	stats.intr_total -= d->initial.intr_total;
	stats.processes -= d->initial.processes;

	data_add_double(dat, now);
	data_add_int64(dat, stats.cpu_user);
	data_add_int64(dat, stats.cpu_nice);
	data_add_int64(dat, stats.cpu_system);
	data_add_int64(dat, stats.cpu_idle);
	data_add_int64(dat, stats.cpu_iowait);
	data_add_int64(dat, stats.cpu_irq);
	data_add_int64(dat, stats.cpu_softirq);
	data_add_int64(dat, stats.cpu_steal);
	data_add_int64(dat, stats.cpu_guest);
	data_add_int64(dat, stats.cpu_guest_nice);
	data_add_int64(dat, stats.intr_total);
	data_add_int64(dat, stats.ctxt);
	data_add_int64(dat, stats.processes);
	data_add_int64(dat, stats.procs_running);
	data_add_int64(dat, stats.procs_blocked);

	plugin_add_results(plug, dat);

	return 0;
}

static const struct header *monitor_stat_data_hdr(struct plugin *plug)
{
	static const struct header hdr[] = {
		{
			.name = "time",
			.unit = "s",
			.description = "Time of this measurement.",
		}, {
			.name = "cpu_user",
			.description = "Normal processes executing in user mode.",
		}, {
			.name = "cpu_nice",
			.description = "Niced processes executing in user mode.",
		}, {
			.name = "cpu_system",
			.description = "Processes executing in kernel mode.",
		}, {
			.name = "cpu_idle",
			.description = "Twiddling thumbs.",
		}, {
			.name = "cpu_iowait",
			.description = "Waiting for I/O to complete.",
		}, {
			.name = "cpu_irq",
			.description = "Servicing interrupts.",
		}, {
			.name = "cpu_softirq",
			.description = "Servicing softirqs.",
		}, {
			.name = "cpu_steal",
			.description = "Involuntary wait.",
		}, {
			.name = "cpu_guest",
			.description = "Running a normal guest.",
		}, {
			.name = "cpu_guest_nice",
			.description = "Running a niced guest.",
		}, {
			.name = "interrupts",
			.description = "Total number of interrupts.",
		}, {
			.name = "contextswitches",
			.description = "Total number of contextswitches.",
		}, {
			.name = "processes",
			.description = "Total number of processes.",
		}, {
			.name = "procs_running",
			.description = "Number of running processes.",
		}, {
			.name = "procs_blocked",
			.description = "Number of blocked processes.",
		}, {
			/* Sentinel */
		}
	};

	return hdr;
}

static int monitor_stat_mod_init(struct module *mod,
				 const struct plugin_id *plug)
{
	if (!access(monitor_stats_path, F_OK | R_OK))
		plugin_monitor_stats_requirements[0].found = 1;

	return 0;
}

const struct plugin_id plugin_stat = {
	.name = "monitor-stat",
	.description = "Monitor plugin to keep track of different values shown in /proc/stat.",
	.module_init = monitor_stat_mod_init,
	.install = monitor_stat_install,
	.uninstall = monitor_stat_uninstall,
	.init = monitor_stat_init,
	.monitor = monitor_stat_mon,
	.versions = plugin_monitor_stats_versions,
	.data_hdr = monitor_stat_data_hdr,
};
