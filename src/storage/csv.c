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
	if (ret) {
		free(buf);
		return NULL;
	}

	data = malloc(sizeof(*data));
	if (!data)
		return NULL;

	data->path = path;

	return data;
}

static int csv_add_sysinfo(void *storage, struct system *sys)
{
	FILE *f;
	char *buf = NULL;
	size_t buf_len;
	int ret;
	struct csv_data *data = (struct csv_data *) storage;
	char *path_buf;

	path_buf = malloc(strlen(data->path) + 256);
	if (!path_buf)
		return -1;

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

	ret = system_info_comma_hdr(sys, &buf, &buf_len);
	if (ret) {
		printk(KERN_ERR "System info header failed\n");
		goto error;
	}
	fwrite(buf, 1, strlen(buf), f);
	fwrite("\n", 1, 1, f);

	ret = system_info_comma_str(sys, &buf, &buf_len);
	if (ret) {
		printk(KERN_ERR "System info content failed\n");
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

		ret = system_cpu_comma_hdr(sys->hw.cpus, &buf, &buf_len);
		if (ret) {
			printk(KERN_ERR "Failed getting cpu header\n");
			goto error;
		}
		fwrite(buf, 1, strlen(buf), f);
		fwrite("\n", 1, 1, f);

		for (i = 0; i != sys->hw.nr_cpus; ++i) {
			ret = system_cpu_comma_str(&sys->hw.cpus[i], &buf,
							&buf_len);
			if (ret) {
				printk(KERN_ERR "Failed getting sysinfo for cpu %d\n",
						i);
				goto error;
			}
			fwrite(buf, 1, strlen(buf), f);
			fwrite("\n", 1, 1, f);
		}
		fclose(f);
	}

	free(path_buf);

	return 0;
error:
	if (f)
		fclose(f);
	free(path_buf);
	if (buf)
		free(buf);
	return -1;
}

static int csv_add_data(void *storage, struct plugin *plug, struct data *data)
{
	return 0;
}

static void csv_exit(void *storage)
{
	free(storage);
}

struct storage storage_csv = {
	.init = csv_init,
	.add_sysinfo = csv_add_sysinfo,
	.add_data = csv_add_data,
	.exit = csv_exit,
};
