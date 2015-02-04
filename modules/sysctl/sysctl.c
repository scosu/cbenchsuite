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

extern const struct plugin_id plugin_drop_caches;
extern const struct plugin_id plugin_swap_reset;
extern const struct plugin_id plugin_stat;
extern const struct plugin_id plugin_meminfo;
extern const struct plugin_id plugin_schedstat;

static const struct plugin_id *plugins[] = {
	&plugin_drop_caches,
	&plugin_swap_reset,
	&plugin_stat,
	&plugin_meminfo,
	&plugin_schedstat,
	NULL
};

struct module_id sysctl_module = {
	.plugins = plugins,
};
MODULE_REGISTER(sysctl_module);
