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

#include <cbench/storage/csv.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <klib/printk.h>

#include <cbench/data.h>
#include <cbench/plugin.h>
#include <cbench/storage.h>
#include <cbench/system.h>

struct csv_data {
	const char *path;
	const char *plg_grp_sha256;
	char *path_buf;
};

static void *csv_init(const char *path)
{
	struct csv_data *data;
	int ret;

	char *buf = malloc(2 * strlen(path) + 128);
	if (!buf)
		return NULL;

	sprintf(buf, "mkdir -p "
			"%s/system "
			"%s/system/cpus ", path, path);
	ret = system(buf);
	free(buf);
	if (ret) {
		return NULL;
	}

	data = malloc(sizeof(*data));
	if (!data)
		return NULL;

	data->path = path;

	data->path_buf = malloc(strlen(data->path) + 256);
	if (!data->path_buf) {
		free(data);
		return NULL;
	}
	return data;
}

static int csv_add_sysinfo(void *storage, struct system *sys)
{
	FILE *f;
	char *buf = NULL;
	size_t buf_len;
	int ret;
	struct csv_data *data = (struct csv_data *) storage;
	char *path_buf = data->path_buf;
	struct data *sys_dat;
	const struct value *vals;

	sprintf(path_buf, "%s/system/%s", data->path, sys->sha256);

	f = fopen(path_buf, "r");
	if (f) {
		fclose(f);
		free(path_buf);
		return 0;
	}

	f = fopen(path_buf, "w");
	if (!f) {
		printk(KERN_ERR "Failed opening file %s for writing", path_buf);
		goto error;
	}

	vals = system_info_hdr(sys);
	if (!vals) {
		printk(KERN_ERR "Failed getting system info header\n");
		goto error;
	}

	ret = values_to_csv(vals, &buf, &buf_len);
	if (ret) {
		printk(KERN_ERR "Failed translating values to csv\n");
		goto error;
	}
	fwrite(buf, 1, strlen(buf), f);
	fwrite("\n", 1, 1, f);

	sys_dat = system_info_data(sys);
	if (!sys_dat) {
		printk(KERN_ERR "Failed to get system info data\n");
		goto error;
	}

	ret = data_to_csv(sys_dat, &buf, &buf_len);
	data_put(sys_dat);
	if (ret) {
		printk(KERN_ERR "Failed translating system data to csv\n");
		goto error;
	}
	fwrite(buf, 1, strlen(buf), f);
	fwrite("\n", 1, 1, f);
	fclose(f);


	sprintf(path_buf, "%s/system/cpus/%s", data->path, sys->hw.cpus_sha256);

	f = fopen(path_buf, "r");
	if (f) {
		fclose(f);
	} else {
		int i;

		f = fopen(path_buf, "w");
		if (!f) {
			printk(KERN_ERR "Failed opening file %s\n", path_buf);
			goto error;
		}

		vals = system_cpu_hdr(sys->hw.cpus);
		if (!vals) {
			printk(KERN_ERR "Failed to get cpu header\n");
			goto error;
		}

		ret = values_to_csv(vals, &buf, &buf_len);
		if (ret) {
			printk(KERN_ERR "Failed translating cpu header\n");
			goto error;
		}
		fwrite(buf, 1, strlen(buf), f);
		fwrite("\n", 1, 1, f);

		for (i = 0; i != sys->hw.nr_cpus; ++i) {
			sys_dat = system_cpu_data(&sys->hw.cpus[i]);
			if (!sys_dat) {
				printk(KERN_ERR "Failed getting sysinfo for cpu %d\n",
						i);
				goto error;
			}

			ret = data_to_csv(sys_dat, &buf, &buf_len);
			data_put(sys_dat);
			if (ret) {
				printk(KERN_ERR "Failed translating cpu sysinfo\n");
				goto error;
			}

			fwrite(buf, 1, strlen(buf), f);
			fwrite("\n", 1, 1, f);
		}
		fclose(f);
	}

	return 0;
error:
	if (f)
		fclose(f);
	if (buf)
		free(buf);
	return -1;
}

static int csv_plugin_combo_init(void *storage, struct list_head *plugins,
		const char *plugins_sha256)
{
	struct csv_data *data = (struct csv_data *) storage;
	char *path_buf = data->path_buf;
	const char grp_hdr[] = "plugin\n";
	struct plugin *plg;
	FILE *f;

	sprintf(path_buf, "%s/plugin_groups/%s/group.csv", data->path, plugins_sha256);

	f = fopen(path_buf, "r");
	if (!f) {
		f = fopen(path_buf, "w");
		if (!f) {
			printk(KERN_ERR "Failed opening %s\n", path_buf);
			return -1;
		}
		fwrite(grp_hdr, 1, strlen(grp_hdr), f);

		list_for_each_entry(plg, plugins, plugin_grp) {
			fwrite(plg->sha256, 1, strlen(plg->sha256), f);
			fwrite("\n", 1, 1, f);
		}
		fclose(f);
	}

	return 0;
}

static int csv_add_data(void *storage, struct plugin *plug, struct data *data)
{
	return 0;
}

static void csv_exit(void *storage)
{
	struct csv_data *data = (struct csv_data *) storage;
	free(data->path_buf);
	free(storage);
}

const struct storage_ops storage_csv = {
	.init = csv_init,
	.init_plugin_grp = csv_plugin_combo_init,
	.add_sysinfo = csv_add_sysinfo,
	.add_data = csv_add_data,
	.exit = csv_exit,
};
