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

#include <cbench/storage/sqlite3.h>

#include <stddef.h>
#include <stdio.h>

#include <sqlite3.h>

#include <klib/list.h>
#include <klib/printk.h>

#include <cbench/data.h>
#include <cbench/module.h>
#include <cbench/option.h>
#include <cbench/plugin.h>
#include <cbench/sha256.h>
#include <cbench/system.h>
#include <cbench/util.h>
#include <cbench/version.h>

struct sqlite3_data {
	sqlite3 *db;
	char *stmt;
	size_t stmt_size;
	char *buf1;
	size_t buf1_size;
	char *buf2;
	size_t buf2_size;
	const char *group_sha;
	const char *sys_sha;
	const char *run_uuid;
};

struct values_present {
	const struct header *vals;
	int *present;
};

static int sqlite3_alter_by_hdr_cb(void *ctx, int nr_col, char **cols,
					char **names)
{
	struct values_present *vp = (struct values_present *) ctx;
	int i;

	for (i = 0; vp->vals[i].name != NULL; ++i) {
		if (!strcmp(vp->vals[i].name, cols[1])) {
			vp->present[i] = 1;
			break;
		}
	}
	return 0;
}

/*
 * WARNING: This internal function uses buf2 and stmt buffers.
 */
static int sqlite3_alter_by_hdr(const char *table, struct sqlite3_data *d,
				const struct header *hdr, const char *additional_hdr)
{
	struct values_present vp;
	int nr_vals;
	int i;
	char **stmt = &d->stmt;
	size_t *stmt_size = &d->stmt_size;
	char **buf = &d->buf2;
	size_t *buf_len = &d->buf2_size;
	char *errmsg;
	int ret;
	int sum_found;

	for (i = 0; hdr[i].name != NULL; ++i) ;
	nr_vals = i;

	vp.vals = hdr;
	vp.present = calloc(sizeof(*vp.present), nr_vals);

	if (!vp.present)
		return -1;

	ret = mem_grow((void**)stmt, stmt_size, strlen(table) + 128);
	if (ret) {
		free(vp.present);
		return -1;
	}

	sprintf(*stmt, "PRAGMA table_info('%s');", table);

	ret = sqlite3_exec(d->db, *stmt, sqlite3_alter_by_hdr_cb, &vp, &errmsg);

	if (ret != SQLITE_OK) {
		printk(KERN_ERR "Failed getting table info (%s): %s\n", *stmt,
				errmsg);
		sqlite3_free(errmsg);
		goto error_table_info;
	}

	sum_found = 0;
	for (i = 0; i != nr_vals; ++i)
		sum_found += vp.present[i];
	if (sum_found == nr_vals)
		return 0;
	if (sum_found == 0) {

		ret = header_to_csv(hdr, buf, buf_len, QUOTE_SINGLE);
		if (ret) {
			goto error_table_info;
		}

		ret = mem_grow((void**)stmt, stmt_size, strlen(table)
				+ strlen(*buf)
				+ strlen(additional_hdr)
				+ 128);
		if (ret) {
			free(buf);
			goto error_table_info;
		}

		sprintf(*stmt, "CREATE TABLE '%s'(%s,%s);", table,
				additional_hdr, *buf);
		ret = sqlite3_exec(d->db, *stmt, NULL, NULL, &errmsg);
		if (ret != SQLITE_OK) {
			printk(KERN_ERR "Failed to create table (%s): %s\n",
					*stmt, errmsg);
			sqlite3_free(errmsg);
			goto error_table_info;
		}
	} else {
		for (i = 0; vp.vals[i].name != NULL; ++i) {
			if (vp.present[i])
				continue;
			ret = mem_grow((void**)stmt, stmt_size, strlen(hdr[i].name)
					+ strlen(table) + 128);
			if (ret) {
				goto error_table_info;
			}

			sprintf(*stmt, "ALTER TABLE %s ADD COLUMN '%s';", table,
					vp.vals[i].name);
			ret = sqlite3_exec(d->db, *stmt, NULL, NULL, &errmsg);
			if (ret != SQLITE_OK) {
				printk(KERN_ERR "Failed to add column (%s): %s\n",
						*stmt, errmsg);
				sqlite3_free(errmsg);
				goto error_table_info;
			}
		}
	}
	free(vp.present);
	return 0;

error_table_info:
	free(vp.present);
	return -1;
}

static int sqlite3_store_header_metadata(struct sqlite3_data *d,
		const struct header *hdr, const char *sha_name, const char *sha,
		const char *table, const char *join_sha)
{
	int ret;
	char **stmt = &d->stmt;
	size_t *stmt_len = &d->stmt_size;
	char *errmsg;
	int i;

	for (i = 0; hdr[i].name != NULL; ++i) {
		sha256_context ctx;
		char sha_meta[65];

		sha256_starts(&ctx);

		sha256_add_str(&ctx, sha);
		sha256_add_str(&ctx, hdr[i].name);
		if (hdr[i].description)
			sha256_add_str(&ctx, hdr[i].description);
		if (hdr[i].unit)
			sha256_add_str(&ctx, hdr[i].unit);

		sha256_finish_str(&ctx, sha_meta);

		ret = mem_grow((void **)stmt, stmt_len, strlen(table) + strlen(sha_name)
				+ strlen(sha_meta) + strlen(sha)
				+ strlen(hdr[i].name)
				+ (hdr[i].description ? strlen(hdr[i].description) : 0)
				+ (hdr[i].unit ? strlen(hdr[i].unit) : 0) + 128);
		if (ret)
			return -1;

		sprintf(*stmt, "INSERT OR IGNORE INTO %s("
					"%s,"
					"%s,"
					"name,"
					"description,"
					"unit"
				") VALUES("
					"'%s','%s','%s','%s','%s');",
				table,
				join_sha,
				sha_name,
				sha_meta,
				sha,
				hdr[i].name,
				(hdr[i].description ? hdr[i].description : ""),
				(hdr[i].unit ? hdr[i].unit : ""));
		ret = sqlite3_exec(d->db, *stmt, NULL, NULL, &errmsg);
		if (ret != SQLITE_OK) {
			printk(KERN_ERR "Failed to insert metadata into %s (%s)L %s\n",
					table, *stmt, errmsg);
			sqlite3_free(errmsg);
			return -1;
		}
	}
	return 0;
}

static void *sqlite3_init(const char *path)
{
	struct sqlite3_data *d;
	int ret;
	char *errmsg;

	d = malloc(sizeof(*d));
	if (!d)
		return NULL;

	d->stmt = NULL;
	d->buf1 = NULL;
	d->buf2 = NULL;

	ret = mem_grow((void**)&d->buf1, &d->buf1_size, strlen(path) + 64);
	if (ret) {
		goto error;
	}

	sprintf(d->buf1, "mkdir -p %s", path);
	ret = system(d->buf1);
	if (ret) {
		printk(KERN_ERR "Failed to create dir %s\n", d->buf1);
		goto error;
	}

	sprintf(d->buf1, "%s/db.sqlite", path);

	ret = sqlite3_open(d->buf1, &d->db);
	if (ret) {
		printk(KERN_ERR "Failed opening database %s: %s\n", d->buf1,
				sqlite3_errmsg(d->db));
		goto error_sqldb;
	}

	ret = sqlite3_exec(d->db, "CREATE TABLE IF NOT EXISTS plugin("
					"plugin_sha UNIQUE PRIMARY KEY,"
					"module,"
					"name,"
					"description,"
					"version,"
					"plugin_table,"
					"plugin_opt_table,"
					"plugin_comp_vers_table);",
				NULL, NULL, &errmsg);
	if (ret != SQLITE_OK) {
		printk(KERN_ERR "Failed to create plugins table: %s\n",
				errmsg);
		sqlite3_free(errmsg);
		goto error_sqldb;
	}

	ret = sqlite3_exec(d->db, "CREATE TABLE IF NOT EXISTS plugin_group("
					"sha UNIQUE PRIMARY KEY,"
					"plugin_group_sha,"
					"plugin_sha,"
					"plugin_opts_sha,"
					"plugin_comp_vers_sha);",
				NULL, NULL, &errmsg);
	if (ret != SQLITE_OK) {
		printk(KERN_ERR "Failed to create plugin_grp table: %s\n",
				errmsg);
		sqlite3_free(errmsg);
		goto error_sqldb;
	}

	ret = sqlite3_exec(d->db, "CREATE TABLE IF NOT EXISTS unique_run("
					"run_uuid UNIQUE PRIMARY KEY,"
					"plugin_group_sha,"
					"prev_runs,"
					"system_sha);",
				NULL, NULL, &errmsg);
	if (ret != SQLITE_OK) {
		printk(KERN_ERR "Failed to create unique_runs table: %s\n",
				errmsg);
		sqlite3_free(errmsg);
		goto error_sqldb;
	}

	ret = sqlite3_exec(d->db, "CREATE TABLE IF NOT EXISTS plugin_option_meta("
					"plugin_option_meta_sha UNIQUE PRIMARY KEY,"
					"plugin_sha,"
					"name,"
					"description,"
					"unit);",
				NULL, NULL, &errmsg);
	if (ret != SQLITE_OK) {
		printk(KERN_ERR "Failed to create plugin_option_meta table: %s\n",
				errmsg);
		sqlite3_free(errmsg);
		goto error_sqldb;
	}

	ret = sqlite3_exec(d->db, "CREATE TABLE IF NOT EXISTS plugin_data_meta("
					"plugin_data_meta_sha UNIQUE PRIMARY KEY,"
					"plugin_sha,"
					"name,"
					"description,"
					"unit);",
				NULL, NULL, &errmsg);
	if (ret != SQLITE_OK) {
		printk(KERN_ERR "Failed to create plugin_option_meta table: %s\n",
				errmsg);
		sqlite3_free(errmsg);
		goto error_sqldb;
	}

	return d;

error_sqldb:
	sqlite3_close(d->db);
error:
	if (d->buf1)
		free(d->buf1);
	free(d);
	return NULL;
}

static int sqlite3_add_sysinfo(void *storage, struct system *sys)
{
	struct sqlite3_data *d = (struct sqlite3_data *) storage;
	char **stmt = &d->stmt;
	size_t *stmt_len = &d->stmt_size;
	char **buf1 = &d->buf1;
	size_t *buf1_len = &d->buf1_size;
	char **buf2 = &d->buf2;
	size_t *buf2_len = &d->buf2_size;
	char *errmsg;
	const struct header *hdr;
	struct data *dat;
	char system_cpu_sha[65];
	sha256_context ctx;
	int ret;
	int i;

	d->sys_sha = sys->sha256;

	hdr = system_info_hdr(sys);
	if (!hdr) {
		printk(KERN_ERR "Failed to get system header\n");
		return -1;
	}

	ret = sqlite3_alter_by_hdr("system", d, hdr, "cpus_sha,system_sha UNIQUE PRIMARY KEY");
	if (ret) {
		goto error;
	}

	ret = header_to_csv(hdr, buf1, buf1_len, QUOTE_SINGLE);
	if (ret) {
		printk(KERN_ERR "Failed translating system header to csv\n");
		goto error;
	}

	dat = system_info_data(sys);
	if (!dat) {
		printk(KERN_ERR "Failed getting system info data\n");
		goto error;
	}

	ret = data_to_csv(dat, buf2, buf2_len, QUOTE_SINGLE);
	data_put(dat);
	if (ret) {
		printk(KERN_ERR "Failed translating system data to csv\n");
		goto error;
	}

	ret = mem_grow((void**)stmt, stmt_len, strlen(*buf1) + strlen(*buf2)
			+ strlen(sys->sha256)
			+ strlen(sys->hw.cpus_sha256) + 128);
	if (ret)
		goto error;


	sprintf(*stmt, "INSERT OR IGNORE INTO system("
				"%s,"
				"system_sha,"
				"cpus_sha"
			") VALUES("
				"%s,'%s','%s');",
			*buf1,
			*buf2,
			sys->sha256,
			sys->hw.cpus_sha256);
	ret = sqlite3_exec(d->db, *stmt, NULL, NULL, &errmsg);
	if (ret != SQLITE_OK) {
		printk(KERN_ERR "Failed inserting system data (%s): %s\n",
				*stmt, errmsg);
		sqlite3_free(errmsg);
		goto error;
	}

	hdr = system_cpu_type_hdr(sys->hw.cpus);
	if (!hdr) {
		printk(KERN_ERR "Failed to get system cpu type header\n");
		goto error;
	}

	ret = sqlite3_alter_by_hdr("cpu_type", d, hdr, "cpu_type_sha UNIQUE PRIMARY KEY");
	if (ret) {
		printk(KERN_ERR "Failed to alter system_cpus\n");
		goto error;
	}

	ret = header_to_csv(hdr, buf1, buf1_len, QUOTE_SINGLE);
	if (ret) {
		printk(KERN_ERR "Failed to translate system cpu header to csv\n");
		goto error;
	}

	for (i = 0; i != sys->hw.nr_cpus; ++i) {
		struct system_cpu *cpu = &sys->hw.cpus[i];

		dat = system_cpu_type_data(cpu);
		if (!dat) {
			printk(KERN_ERR "Failed to get system cpu type values for %d\n", i);
			goto error;
		}

		ret = data_to_csv(dat, buf2, buf2_len, QUOTE_SINGLE);
		data_put(dat);
		if (ret) {
			printk(KERN_ERR "Failed to translate cpu values for %d\n", i);
			goto error;
		}

		ret = mem_grow((void**)stmt, stmt_len, strlen(*buf1) + strlen(*buf2)
				+ strlen(cpu->type_sha256) + 128);
		if (ret) {
			goto error;
		}

		sprintf(*stmt, "INSERT OR IGNORE INTO cpu_type("
					"%s,"
					"cpu_type_sha"
				") VALUES("
					"%s,'%s');",
				*buf1,
				*buf2,
				cpu->type_sha256);
		ret = sqlite3_exec(d->db, *stmt, NULL, NULL, &errmsg);
		if (ret != SQLITE_OK) {
			printk(KERN_ERR "Failed to insert cpu type (%s): %s\n",
					*stmt, errmsg);
			sqlite3_free(errmsg);
			goto error;
		}
	}

	hdr = system_cpu_hdr(sys->hw.cpus);
	if (!hdr) {
		printk(KERN_ERR "Failed to get system cpu type header\n");
		goto error;
	}

	ret = sqlite3_alter_by_hdr("system_cpu", d, hdr, "sha UNIQUE PRIMARY KEY,cpus_sha,cpu_type_sha");
	if (ret) {
		printk(KERN_ERR "Failed to alter system_cpus\n");
		goto error;
	}

	ret = header_to_csv(hdr, buf1, buf1_len, QUOTE_SINGLE);
	if (ret) {
		printk(KERN_ERR "Failed to translate system cpu header to csv\n");
		goto error;
	}

	for (i = 0; i != sys->hw.nr_cpus; ++i) {
		struct system_cpu *cpu = &sys->hw.cpus[i];

		sha256_starts(&ctx);
		sha256_add_str(&ctx, sys->hw.cpus_sha256);
		sha256_add_str(&ctx, cpu->type_sha256);
		sha256_add_str(&ctx, cpu->sha256);
		sha256_finish_str(&ctx, system_cpu_sha);

		dat = system_cpu_data(cpu);
		if (!dat) {
			printk(KERN_ERR "Failed to get system cpu type values for %d\n", i);
			goto error;
		}

		ret = data_to_csv(dat, buf2, buf2_len, QUOTE_SINGLE);
		data_put(dat);
		if (ret) {
			printk(KERN_ERR "Failed to translate cpu values for %d\n", i);
			goto error;
		}

		ret = mem_grow((void**)stmt, stmt_len, strlen(*buf1) + strlen(*buf2)
				+ strlen(cpu->type_sha256)
				+ strlen(system_cpu_sha)
				+ strlen(cpu->sha256) + 128);
		if (ret) {
			goto error;
		}

		sprintf(*stmt, "INSERT OR IGNORE INTO system_cpu("
					"%s,"
					"cpus_sha,"
					"sha,cpu_type_sha"
				") VALUES("
					"%s,'%s','%s','%s');",
				*buf1,
				*buf2,
				sys->hw.cpus_sha256,
				cpu->sha256,
				cpu->type_sha256);
		ret = sqlite3_exec(d->db, *stmt, NULL, NULL, &errmsg);
		if (ret != SQLITE_OK) {
			printk(KERN_ERR "Failed to insert cpu (%s): %s\n",
					*stmt, errmsg);
			sqlite3_free(errmsg);
			goto error;
		}
	}
	return 0;
error:
	return -1;
}

static int sqlite3_plugin_store_options(struct sqlite3_data *d,
		struct plugin *plug)
{
	int ret;
	struct header *opts = plug->options;
	char **buf1 = &d->buf1;
	size_t *buf1_len = &d->buf1_size;
	char **buf2 = &d->buf2;
	size_t *buf2_len = &d->buf2_size;
	char **stmt = &d->stmt;
	size_t *stmt_size = &d->stmt_size;
	char *errmsg;

	ret = sqlite3_store_header_metadata(d, opts, "plugin_sha",
			plug->sha256, "plugin_option_meta", "plugin_option_meta_sha");
	if (ret)
		return -1;

	ret = option_to_hdr_csv(opts, buf1, buf1_len, QUOTE_SINGLE);
	if (ret) {
		printk(KERN_ERR "Failed to translate option header to csv\n");
		return -1;
	}

	ret = mem_grow((void**)stmt, stmt_size,
			strlen(plug->mod->name)
			+ strlen(plug->id->name)
			+ strlen(plug->version->version)
			+ strlen(*buf1)
			+ 128);
	if (ret)
		return -1;

	sprintf(*stmt, "CREATE TABLE IF NOT EXISTS 'plugin_opts_%s__%s__%s'("
				"plugin_opts_sha UNIQUE PRIMARY KEY,"
				"%s);",
			plug->mod->name,
			plug->id->name,
			plug->version->version,
			*buf1);
	ret = sqlite3_exec(d->db, *stmt, NULL, NULL, &errmsg);
	if (ret != SQLITE_OK) {
		printk(KERN_ERR "Failed creating option table (%s): %s",
				*stmt, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}

	ret = option_to_data_csv(opts, buf2, buf2_len, QUOTE_SINGLE);
	if (ret) {
		printk(KERN_ERR "Failed to translate option data to csv\n");
		return -1;
	}

	ret = mem_grow((void**)stmt, stmt_size,
			strlen(plug->mod->name)
			+ strlen(plug->id->name)
			+ strlen(plug->version->version)
			+ strlen(*buf1)
			+ strlen(*buf2)
			+ strlen(plug->opt_sha256)
			+ 128);
	if (ret)
		return -1;
	sprintf(*stmt, "INSERT OR IGNORE INTO \"plugin_opts_%s__%s__%s\"("
				"plugin_opts_sha,"
				"%s"
			") VALUES("
				"'%s',%s);",
			plug->mod->name,
			plug->id->name,
			plug->version->version,
			*buf1,
			plug->opt_sha256,
			*buf2);
	ret = sqlite3_exec(d->db, *stmt, NULL, NULL, &errmsg);
	if (ret != SQLITE_OK) {
		printk(KERN_ERR "Failed inserting plugin options (%s): %s\n",
				*stmt, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}
	return 0;
}

static int sqlite3_plugin_store_versions(struct sqlite3_data *d,
		struct plugin *plug)
{
	int ret;
	char **buf1 = &d->buf1;
	size_t *buf1_len = &d->buf1_size;
	char **buf2 = &d->buf2;
	size_t *buf2_len = &d->buf2_size;
	char **stmt = &d->stmt;
	size_t *stmt_size = &d->stmt_size;
	char *errmsg;
	struct comp_version *vers = plug->version->comp_versions;

	ret = comp_versions_to_csv(vers, buf1, buf1_len, QUOTE_SINGLE);
	if (ret) {
		printk(KERN_ERR "Failed to translate comp versions to csv\n");
		return -1;
	}

	ret = mem_grow((void**)stmt, stmt_size,
			strlen(plug->mod->name)
			+ strlen(plug->id->name)
			+ strlen(plug->version->version)
			+ strlen(*buf1)
			+ 128);
	if (ret)
		return -1;

	sprintf(*stmt, "CREATE TABLE IF NOT EXISTS 'plugin_comp_vers_%s__%s__%s'("
				"plugin_comp_vers_sha UNIQUE PRIMARY KEY,"
				"%s);",
			plug->mod->name,
			plug->id->name,
			plug->version->version,
			*buf1);
	ret = sqlite3_exec(d->db, *stmt, NULL, NULL, &errmsg);
	if (ret != SQLITE_OK) {
		printk(KERN_ERR "Failed creating option table (%s): %s",
				*stmt, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}

	ret = comp_versions_to_data_csv(vers, buf2, buf2_len, QUOTE_SINGLE);
	if (ret) {
		printk(KERN_ERR "Failed to translate version data to csv\n");
		return -1;
	}

	ret = mem_grow((void**)stmt, stmt_size,
			strlen(plug->mod->name)
			+ strlen(plug->id->name)
			+ strlen(plug->version->version)
			+ strlen(*buf1)
			+ strlen(*buf2)
			+ strlen(plug->ver_sha256)
			+ 128);
	if (ret)
		return -1;
	sprintf(*stmt, "INSERT OR IGNORE INTO \"plugin_comp_vers_%s__%s__%s\"("
				"plugin_comp_vers_sha,"
				"%s"
			") VALUES("
				"'%s',%s);",
			plug->mod->name,
			plug->id->name,
			plug->version->version,
			*buf1,
			plug->ver_sha256,
			*buf2);
	ret = sqlite3_exec(d->db, *stmt, NULL, NULL, &errmsg);
	if (ret != SQLITE_OK) {
		printk(KERN_ERR "Failed inserting plugin versions (%s): %s\n",
				*stmt, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}
	return 0;
}

static int sqlite3_init_plugin_grp(void *storage, struct list_head *plugins,
					const char *group_sha)
{
	struct sqlite3_data *d = storage;
	char **stmt = &d->stmt;
	size_t *stmt_size = &d->stmt_size;
	struct plugin *plug;
	int ret;
	char **buf1 = &d->buf1;
	size_t *buf1_len = &d->buf1_size;
	sha256_context ctx;
	char sha[65];
	char *errmsg;

	d->group_sha = group_sha;

	list_for_each_entry(plug, plugins, plugin_grp) {
		const struct header *hdr;
		const struct header *opts;

		sha256_starts(&ctx);
		opts = plugin_get_options(plug);
		if (opts) {
			ret = sqlite3_plugin_store_options(d, plug);
			if (ret)
				return -1;
			sha256_add_str(&ctx, plug->opt_sha256);
		}

		if (plug->version->comp_versions) {
			ret = sqlite3_plugin_store_versions(d, plug);
			if (ret)
				return -1;
			sha256_add_str(&ctx, plug->ver_sha256);
		}

		sha256_add_str(&ctx, group_sha);
		sha256_add_str(&ctx, plug->sha256);
		sha256_finish_str(&ctx, sha);

		hdr = plugin_data_hdr(plug);
		if (hdr) {
			ret = sqlite3_store_header_metadata(d, hdr, "plugin_sha",
					plug->sha256, "plugin_data_meta", "plugin_data_meta_sha");
			if (ret)
				return -1;

			ret = mem_grow((void**)buf1, buf1_len, strlen(plug->mod->name)
					+ strlen(plug->id->name)
					+ strlen(plug->version->version) + 128);
			if (ret) {
				return -1;
			}

			sprintf(*buf1, "plugin_%s__%s__%s", plug->mod->name,
					plug->id->name,
					plug->version->version);
			ret = sqlite3_alter_by_hdr(*buf1, d, hdr, "run_uuid,type_monitor");
			if (ret) {
				printk(KERN_ERR "Failed to update plugin table %s\n",
						buf1);
				return -1;
			}
		}

		ret = mem_grow((void**)stmt, stmt_size, strlen(plug->sha256)
				+ 4 * strlen(plug->mod->name)
				+ (plug->id->description ? strlen(plug->id->description) : 0)
				+ 4 * strlen(plug->version->version)
				+ 3 * strlen(plug->id->name));
		if (ret)
			return -1;

		sprintf(*stmt, "INSERT OR IGNORE INTO plugin("
					"plugin_sha,"
					"module,"
					"name,"
					"description,"
					"version,"
					"plugin_table,"
					"plugin_opt_table,"
					"plugin_comp_vers_table"
				") VALUES("
					"'%s','%s','%s','%s','%s',"
					"'plugin_table_%s__%s__%s',"
					"'plugin_opts_%s__%s__%s',"
					"'plugin_comp_vers_%s__%s__%s');",
				plug->sha256,
				plug->mod->name,
				plug->id->name,
				plug->id->description ? plug->id->description : "",
				plug->version->version,
				plug->mod->name, plug->id->name, plug->version->version,
				plug->mod->name, plug->id->name, plug->version->version,
				plug->mod->name, plug->id->name, plug->version->version);
		ret = sqlite3_exec(d->db, *stmt, NULL, NULL, &errmsg);
		if (ret != SQLITE_OK) {
			printk(KERN_ERR "Failed to insert into plugins (%s): %s\n",
					*stmt, errmsg);
			sqlite3_free(errmsg);
		}

		ret = mem_grow((void**)stmt, stmt_size,
				+ strlen(group_sha) + strlen(sha)
				+ strlen(plug->sha256)
				+ strlen(plug->opt_sha256) + 128);
		if (ret) {
			return -1;
		}

		sprintf(*stmt, "INSERT OR IGNORE INTO plugin_group("
					"sha,"
					"plugin_group_sha,"
					"plugin_sha,"
					"plugin_opts_sha,"
					"plugin_comp_vers_sha"
				") VALUES("
					"'%s','%s','%s','%s','%s');",
				sha, group_sha,
				plug->sha256,
				plug->opt_sha256,
				plug->ver_sha256);
		ret = sqlite3_exec(d->db, *stmt, NULL, NULL, &errmsg);
		if (ret != SQLITE_OK) {
			printk(KERN_ERR "Failed to insert into plugin_groups (%s): %s\n",
					*stmt, errmsg);
			sqlite3_free(errmsg);
		}
	}

	return 0;
}

static int sqlite3_init_run(void *storage, const char *uuid, int nr_run)
{
	struct sqlite3_data *d = storage;
	char **stmt = &d->stmt;
	size_t *stmt_size = &d->stmt_size;
	int ret;
	char *errmsg;

	d->run_uuid = uuid;

	ret = mem_grow((void**)stmt, stmt_size, strlen(uuid) + strlen(d->group_sha)
				+ strlen(d->sys_sha) + 128);
	if (ret)
		return -1;
	sprintf(*stmt, "INSERT INTO unique_run("
				"run_uuid,"
				"plugin_group_sha,"
				"prev_runs,"
				"system_sha"
			") VALUES("
				"'%s','%s',%d,'%s');",
			uuid,
			d->group_sha,
			nr_run,
			d->sys_sha);

	ret = sqlite3_exec(d->db, *stmt, NULL, NULL, &errmsg);
	if (ret != SQLITE_OK) {
		printk(KERN_ERR "Failed to add unique run (%s): %s\n", *stmt,
				errmsg);
		sqlite3_free(errmsg);
		return -1;
	}

	return 0;
}

static int sqlite3_add_data(void *storage, struct plugin *plug,
		struct data *data)
{
	struct sqlite3_data *d = storage;
	char **stmt = &d->stmt;
	size_t *stmt_size = &d->stmt_size;
	char **buf1 = &d->buf1;
	size_t *buf1_size = &d->buf1_size;
	char **buf2 = &d->buf2;
	size_t *buf2_size = &d->buf2_size;
	char *errmsg;
	int ret;
	const struct header *hdr;


	hdr = plugin_data_hdr(plug);
	if (!hdr) {
		printk(KERN_ERR "Failed retrieving plugin %s data header\n",
				plug->id->name);
		return -1;
	}

	ret = header_to_csv(hdr, buf1, buf1_size, QUOTE_SINGLE);
	if (ret) {
		printk(KERN_ERR "Failed to translate plugin %s header to csv\n",
				plug->id->name);
		return -1;
	}

	ret = data_to_csv(data, buf2, buf2_size, QUOTE_SINGLE);
	if (ret) {
		printk(KERN_ERR "Failed to translate plugin %s data to csv\n",
				plug->id->name);
		return -1;
	}

	ret = mem_grow((void**)stmt, stmt_size, strlen(*buf1) + strlen(*buf2)
			+ strlen(plug->mod->name) + strlen(plug->id->name)
			+ strlen(plug->version->version) + strlen(d->run_uuid)
			+ 128);
	if (ret) {
		return -1;
	}

	sprintf(*stmt, "INSERT INTO 'plugin_%s__%s__%s'("
				"%s,"
				"run_uuid,"
				"type_monitor"
			") VALUES("
				"%s,'%s',%d);",
			plug->mod->name, plug->id->name, plug->version->version,
			*buf1,
			*buf2,
			d->run_uuid,
			(data->type == DATA_TYPE_MONITOR));
	ret = sqlite3_exec(d->db, *stmt, NULL, NULL, &errmsg);
	if (ret) {
		printk(KERN_ERR "Failed to insert plugin %s result into table (%s): %s\n",
				plug->id->name, *stmt, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}
	return 0;
}

static void sqlite3_exit(void *storage)
{
	struct sqlite3_data *d = (struct sqlite3_data *)storage;

	sqlite3_close(d->db);

	if (d->buf1)
		free(d->buf1);
	if (d->buf2)
		free(d->buf2);
	if (d->stmt)
		free(d->stmt);
	free(d);
}

const struct storage_ops storage_sqlite3 = {
	.init = sqlite3_init,
	.init_plugin_grp = sqlite3_init_plugin_grp,
	.add_sysinfo = sqlite3_add_sysinfo,
	.init_run = sqlite3_init_run,
	.add_data = sqlite3_add_data,
	.exit = sqlite3_exit,
};
