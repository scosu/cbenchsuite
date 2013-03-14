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

#include <unistd.h>

#include <cbench/exec_helper.h>
#include <cbench/module.h>
#include <cbench/plugin.h>
#include <cbench/requirement.h>
#include <cbench/version.h>

enum plugins {
	PLUGIN_DROP_CACHES = 0,
	PLUGIN_SWAP_RESET,
	PLUGIN_MONITOR_STAT,
	PLUGIN_MONITOR_MEMINFO,
};

const char vm_drop_caches[] = "/proc/sys/vm/drop_caches";

struct module_id sysctl_module;

#include "sysctl_drop_caches.c"
#include "sysctl_swap_reset.c"
#include "sysctl_monitor_stat.c"
#include "sysctl_monitor_meminfo.c"

static int module_init(struct module *mod)
{
	char *check_swap_args[] = {"which", "swapoff", "swapon", NULL};
	if (!access(vm_drop_caches, F_OK | W_OK))
		sysctl_module.plugins[PLUGIN_DROP_CACHES].versions[0].requirements[0].found = 1;
	if (!subproc_call("which", check_swap_args))
		sysctl_module.plugins[PLUGIN_SWAP_RESET].versions[0].requirements[0].found = 1;
	if (!access(monitor_stats_path, F_OK | R_OK))
		sysctl_module.plugins[PLUGIN_MONITOR_STAT].versions[0].requirements[0].found = 1;
	if (!access(monitor_meminfo_path, F_OK | R_OK))
		sysctl_module.plugins[PLUGIN_MONITOR_MEMINFO].versions[0].requirements[0].found = 1;
	return 0;
}

static struct plugin_id plugins[] = {
	{
		.name = "drop-caches",
		.description = "Helper plugin to increase benchmarking accuracy by dropping caches between runs. You have configure in which function slot the caches are dropped.",
		PLUGIN_ALL_FUNCS(drop_caches_func),
		.versions = plugin_drop_caches_versions,
	}, {
		.name = "swap-reset",
		.description = "Helper plugin to increase benchmarking accuracy by resetting the swap between runs. You have to configure in which function slot the swap is reset. Make sure your swap is configured in fstab so that swapoff/swapon doesn't  change the swap setup.",
		PLUGIN_ALL_FUNCS(swapreset_func),
		.versions = plugin_swapreset_versions,
	}, {
		.name = "monitor-stat",
		.description = "Monitor plugin to keep track of different values shown in /proc/stat.",
		.install = monitor_stat_install,
		.uninstall = monitor_stat_uninstall,
		.init = monitor_stat_init,
		.monitor = monitor_stat_mon,
		.versions = plugin_monitor_stats_versions,
		.data_hdr = monitor_stat_data_hdr,
	}, {
		.name = "monitor-meminfo",
		.description = "Monitor plugin to keep track of different values shown in /proc/meminfo.",
		.install = monitor_meminfo_install,
		.uninstall = monitor_meminfo_uninstall,
		.init = monitor_meminfo_init,
		.monitor = monitor_meminfo_mon,
		.versions = plugin_monitor_meminfo_versions,
		.data_hdr = monitor_meminfo_data_hdr,
	}, {
		/* Sentinel */
	}
};

struct module_id sysctl_module = {
	.init = module_init,
	.plugins = plugins,
};
MODULE_REGISTER(sysctl_module);
