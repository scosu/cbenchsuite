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

static struct requirement plugin_swapreset_requirements[] = {
	{
		.name = "swapoff/swapon",
		.description = "This plugin uses swapoff/on to reset swap.",
		.found = 0,
	},
	{ }
};

struct header plugin_swapreset_defaults[] = {
	OPTIONS_BOOL_EACH_FUNC(0),
	OPTION_SENTINEL
};

static struct version plugin_swapreset_versions[] = {
	{
		.version = "0.1",
		.requirements = plugin_swapreset_requirements,
		.default_options = plugin_swapreset_defaults,
	},
	{
		/* Sentinel */
	}
};

static int swapreset()
{
	char *swapoff[] = {"swapoff", "-a", NULL};
	char *swapon[] = {"swapon", "-a", NULL};
	int ret;

	ret = subproc_call("swapoff", swapoff);
	ret |= subproc_call("swapon", swapon);

	return ret;
}

static int swapreset_func(struct plugin *plug)
{
	if (!plugin_exec_func_opt_set(plug))
		return 0;
	return swapreset();
}
