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

#include <stdio.h>
#include <stdlib.h>

#include <cbench/option.h>
#include <cbench/plugin_id_helper.h>

static struct requirement plugin_drop_caches_requirements[] = {
	{
		.name = vm_drop_caches,
		.description = "This plugin needs write permissions for this file to drop caches",
		.found = 0,
	},
	{ }
};

struct header plugin_drop_caches_defaults[] = {
	OPTIONS_BOOL_EACH_FUNC(0),
	OPTION_SENTINEL
};

static struct version plugin_drop_caches_versions[] = {
	{
		.version = "0.1",
		.requirements = plugin_drop_caches_requirements,
		.default_options = plugin_drop_caches_defaults,
	},
	{
		/* Sentinel */
	}
};

static int drop_caches()
{
	FILE *fd;
	int ret = 0;

	fd = fopen(vm_drop_caches, "w");
	if (fd == NULL)
		return 1;
	ret = fwrite("3\n", 1, 2, fd);
	fclose(fd);
	if (ret != 2)
		return 1;
	return 0;
}

static int drop_caches_func(struct plugin *plug)
{
	if (!plugin_exec_func_opt_set(plug))
		return 0;
	return drop_caches();
}

#undef DROP_CACHES_FUNC
