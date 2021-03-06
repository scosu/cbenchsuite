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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <klib/printk.h>

#include <cbench/core/module_manager.h>

#include <cbench/benchsuite.h>
#include <cbench/environment.h>
#include <cbench/plugin.h>
#include <cbench/sha256.h>
#include <cbench/storage.h>
#include <cbench/storage/csv.h>
#include <cbench/storage/sqlite3.h>
#include <cbench/system.h>
#include <cbench/version.h>

#include <cbench_config.h>

struct arguments {
	const char *log_level;
	const char *storage;
	const char *db_path;
	const char *module_dir;
	const char *work_dir;
	const char *download_dir;
	const char *custom_sysinfo;

	const char *warmup_runs;
	const char *min_runs;
	const char *max_runs;
	const char *min_runtime;
	const char *max_runtime;
	const char *std_err;
	const char *skip;

	int cmd_list;
	int cmd_plugins;
	int cmd_help;
	int cmd_continue;
	int verbose;
};

void print_help()
{
	fputs(
"This is cbenchsuite, a high accuracy C benchmark suite.\n\
\n\
cbenchsuite should be executed as root to be able to set different priorities for\n\
important worker threads. Be aware that you should review third-party modules\n\
before using them as root.\n\
\n\
By default, cbenchsuite interpretes arguments without '-' at the beginning as\n\
benchsuite names and executes them.\n\
\n\
Commands:\n\
	--list,-l		List available Modules/Plugins. All other\n\
				command line arguments are parsed as module\n\
				identifiers. If non is given, it lists all\n\
				modules. This command will show more information\n\
				with the verbose option.\n\
	--plugins,-p		Parse all arguments as plugin identifiers\n\
				and execute them. Please see\n\
				doc/identifier_specifications for more\n\
				information.\n\
	--continue,-c		Continue the last command on the given database.\n\
				This will use the log file to start the command\n\
				again and skip all finished executions.\n\
	--help,-h		Displays this help and exits.\n\
\n\
Options:\n\
	--log-level,-g N 	Log level used, from 1 to 7(debugging)\n\
	--storage,-s STR	Storage backend to use. Currently available are:\n\
					sqlite3 and csv\n\
	--db-path,-db DB	Database directory. Default: " CONFIG_DB_DIR "\n\
	--verbose,-v		Verbose output. (more information, but not the\n\
				same as log-level)\n\
	--module-dir,-m PATH	Module directory. Default: " CONFIG_MODULE_DIR "\n\
	--work-dir,-w PATH	Working directory. IMPORTANT! Depending on the\n\
				location of this work directory, the benchmark\n\
				results could vary. Default: " CONFIG_WORK_DIR "\n\
	--download-dir,-d PATH	Download directory. This is one central download\n\
				directory for all modules and plugins.\n\
				Default: " CONFIG_DOWNLOAD_DIR "\n\
	--sysinfo,-i INFO	Additional system information. This will\n\
				seperate the results of the executed benchmarks\n\
				from others. For example you can use any code\n\
				revision or simple names for different test\n\
				conditions.\n\
	--min-runs N		Minimum number of runs executed.\n\
	--max-runs N		Maximum number of runs executed.\n\
	--min-runtime SECONDS	Minimum runtime per independent result value.\n\
	--max-runtime SECONDS	Maximum runtime per independent result value.\n\
	--warmup-runs N		Number of warmup runs before the actual\n\
				measurements.\n\
	--stderr N		Percent of the standard error that need to be\n\
				reached within the other runtime bounds. (float)\n\
	--skip N 		Skip N groups of the execution.\n\
", stdout);
}

#define arg_match(arg, name, key) (!strcmp(arg, name) || !strcmp(arg, key))

int args_parse(struct arguments *pargs, int argc, char **argv)
{
	int i;
	const char **parse_arg_tgt = NULL;

	for (i = 1; i != argc; ++i) {
		char *arg = argv[i];

		if (parse_arg_tgt) {
			*parse_arg_tgt = arg;
			parse_arg_tgt = NULL;
		} else if (arg_match(arg, "--log-level", "-g")) {
			parse_arg_tgt = &pargs->log_level;
		} else if (arg_match(arg, "--storage", "-s")) {
			parse_arg_tgt = &pargs->storage;
		} else if (arg_match(arg, "--db-path", "-db")) {
			parse_arg_tgt = &pargs->db_path;
		} else if (arg_match(arg, "--list", "-l")) {
			pargs->cmd_list = 1;
		} else if (arg_match(arg, "--verbose", "-v")) {
			++pargs->verbose;
		} else if (arg_match(arg, "--module-dir", "-m")) {
			parse_arg_tgt = &pargs->module_dir;
		} else if (arg_match(arg, "--work-dir", "-w")) {
			parse_arg_tgt = &pargs->work_dir;
		} else if (arg_match(arg, "--download-dir", "-d")) {
			parse_arg_tgt = &pargs->download_dir;
		} else if (arg_match(arg, "--plugins", "-p")) {
			pargs->cmd_plugins = 1;
		} else if (arg_match(arg, "--help", "-h")) {
			pargs->cmd_help = 1;
		} else if (arg_match(arg, "--continue", "-c")) {
			pargs->cmd_continue = 1;
		} else if (arg_match(arg, "--sysinfo", "-i")) {
			parse_arg_tgt = &pargs->custom_sysinfo;
		} else if (!strcmp(arg, "--warmup-runs")) {
			parse_arg_tgt = &pargs->warmup_runs;
		} else if (!strcmp(arg, "--min-runs")) {
			parse_arg_tgt = &pargs->min_runs;
		} else if (!strcmp(arg, "--max-runs")) {
			parse_arg_tgt = &pargs->max_runs;
		} else if (!strcmp(arg, "--min-runtime")) {
			parse_arg_tgt = &pargs->min_runtime;
		} else if (!strcmp(arg, "--max-runtime")) {
			parse_arg_tgt = &pargs->max_runtime;
		} else if (!strcmp(arg, "--stderr")) {
			parse_arg_tgt = &pargs->std_err;
		} else if (!strcmp(arg, "--skip")) {
			parse_arg_tgt = &pargs->skip;
		} else if (*arg == '-') {
			printk(KERN_ERR "Unknown option '%s'\n", arg);
			return -1;
		} else {
			continue;
		}

		argv[i] = NULL;
	}
	if (parse_arg_tgt) {
		printk(KERN_ERR "Missing argument for the last option\n");
		return -1;
	}
	return 0;
}

#undef arg_match

const char **create_version_rules(char *arg) {
	int nr_rules = 1;
	int i;
	char *vs;
	char *vs_next;
	const char **rules;

	for (vs = arg; vs && (u64)vs != 1 && vs[0]; vs = strchr(vs, '@') + 1)
		++nr_rules;

	rules = malloc(sizeof(*rules) * nr_rules + 1);
	if (!rules) {
		return NULL;
	}
	memset(&rules[nr_rules], 0, sizeof(*rules));

	for (vs = arg, i = 0; vs && vs[0]; vs = vs_next, ++i) {
		vs_next = strchr(vs, '@');
		if (vs_next && *vs_next == '@') {
			*vs_next = '\0';
			++vs_next;
		}

		rules[i] = vs;
	}
	return rules;
}

int create_plugin_link(struct plugin_link *plug, char *arg)
{
	char *vers_start;
	char *opt_start = NULL;
	plug->name = arg;

	vers_start = strchr(arg, '@');
	if (!vers_start) {
		plug->version_rules = NULL;
		opt_start = strchr(arg, ':');
		if (opt_start) {
			*opt_start = '\0';
			++opt_start;
		}
	} else {
		char *vers_end = strchr(vers_start, ':');
		*vers_start = '\0';
		++vers_start;
		if (vers_end) {
			*vers_end = '\0';
			opt_start = vers_end + 1;
		}
		if (vers_start[0]) {
			plug->version_rules = create_version_rules(vers_start);
			if (!plug->version_rules) {
				printk("Failed to construct version rules for %s\n",
						plug->name);
				return -1;
			}
		} else {
			plug->version_rules = NULL;
		}
	}

	if (opt_start && opt_start[0]) {
		plug->options = opt_start;
	}
	return 0;
}

void put_plugin_link(struct plugin_link *plug)
{
	if (plug->version_rules)
		free(plug->version_rules);
}

struct plugin_link *create_run_combo(char *arg)
{
	char *rc;
	char *rc_next;
	int nr_plugs = 1;
	int i;
	int ret;
	struct plugin_link *plugs;

	for (rc = arg; rc && (u64)rc != 1 && rc[0]; rc = strchr(rc, ';') + 1)
		++nr_plugs;

	plugs = malloc(sizeof(*plugs) * (nr_plugs + 1));
	if (!plugs)
		return NULL;
	memset(plugs, 0, sizeof(*plugs) * (nr_plugs + 1));

	for (rc = arg, i = 0; rc && rc[0]; rc = rc_next, ++i) {
		rc_next = strchr(rc, ';');
		if (rc_next && *rc_next == ';') {
			rc_next[0] = '\0';
			++rc_next;
		}

		ret = create_plugin_link(&plugs[i], rc);
		if (ret) {
			printk(KERN_ERR "Failed to create plugin link\n");
			goto error;
		}
	}

	return plugs;

error:
	if (i) {
		while (i--) {
			put_plugin_link(&plugs[i]);
		}
	}
	free(plugs);
	return NULL;
}

void put_run_combo(struct plugin_link *c)
{
	int i;
	for (i = 0; c[i].name != NULL; ++i)
		put_plugin_link(&c[i]);
	free(c);
}
int execute_benchsuite_args(struct mod_mgr *mm, struct environment *env, int skip,
		struct arguments *pargs, int argc, char **argv)
{
	int i;
	int ret;

	for (i = 1; i != argc; ++i) {
		const char **vers;
		char *ver_string;
		struct benchsuite *suite;
		if (!argv[i])
			continue;

		ver_string = strchr(argv[i], '@');
		if (ver_string) {
			*ver_string = '\0';
			++ver_string;
			vers = create_version_rules(ver_string);
		} else {
			vers = NULL;
		}

		suite = mod_mgr_benchsuite_create(mm, argv[i], vers);
		if (!suite) {
			printk(KERN_ERR "Failed to create benchsuite %s\n",
					argv[i]);
			if (vers)
				free(vers);
			return -1;
		}

		ret = benchsuite_execute(mm, env, suite, &skip);

		if (vers)
			free(vers);

		if (ret) {
			return -1;
		}
	}

	return 0;
}

int execute_cmd_args(struct mod_mgr *mm, struct environment *env, int skip,
		struct arguments *pargs, int argc, char **argv)
{
	int ret = 0;
	int i;
	int ct = 0;
	int nr_combos = 0;
	struct benchsuite *suite;
	struct benchsuite_id *suite_id;
	struct plugin_link **combos;
	for (i = 1; i != argc; ++i) {
		if (argv[i])
			++nr_combos;
	}

	if (!nr_combos) {
		printk(KERN_ERR "No run combinations given\n");
		return -1;
	}

	suite = malloc(sizeof(*suite));
	if (!suite) {
		return -1;
	}
	memset(suite, 0, sizeof(*suite));

	suite_id = malloc(sizeof(*suite_id));
	if (!suite_id) {
		ret = -1;
		goto error_suite_id;
	}
	memset(suite_id, 0, sizeof(*suite_id));
	suite_id->name = "cmd_line_benchsuite";
	suite_id->version.version = "1.0";

	combos = malloc(sizeof(*combos) * (nr_combos + 1));
	if (!combos) {
		ret = -1;
		goto error_combos;
	}
	memset(combos, 0, sizeof(*combos) * (nr_combos + 1));

	for (i = 1; i != argc; ++i) {
		char *arg = argv[i];

		if (!arg)
			continue;

		combos[ct] = create_run_combo(arg);
		if (!combos[ct]) {
			printk(KERN_ERR "Failed to create runcombination from %s\n",
					arg);
			ret = -1;
			goto error_combo_loop;
		}

		++ct;
	}

	suite_id->plugin_grps = combos;
	suite->mod = NULL;
	suite->id = suite_id;

	ret = benchsuite_execute(mm, env, suite, &skip);

error_combo_loop:
	if (ct) {
		while (ct--) {
			put_run_combo(combos[ct]);
		}
	}
	free(combos);
error_combos:
	free(suite_id);
error_suite_id:
	free(suite);
	return ret;
}

int cmd_execute(struct arguments *pargs, int argc, char **argv, int as_benchsuite)
{
	struct system sys;
	struct mod_mgr mm;
	int ret;
	int skip = 0;
	struct environment env = {
		.work_dir = pargs->work_dir,
		.bin_dir = pargs->module_dir,
		.download_dir = pargs->download_dir,
		.settings = {
			.warmup_runs = CONFIG_WARMUP_RUNS,
			.runs_min = CONFIG_MIN_RUNS,
			.runs_max = CONFIG_MAX_RUNS,
			.runtime_min = CONFIG_MIN_RUNTIME,
			.runtime_max = CONFIG_MAX_RUNTIME,
		},
	};
	env.settings.percent_stderr = atof(pargs->std_err);
	if (pargs->min_runs)
		env.settings.runs_min = atoi(pargs->min_runs);
	if (pargs->max_runs)
		env.settings.runs_max = atoi(pargs->max_runs);
	if (pargs->min_runtime)
		env.settings.runtime_min = atoi(pargs->min_runtime);
	if (pargs->max_runtime)
		env.settings.runtime_max = atoi(pargs->max_runtime);
	if (pargs->warmup_runs)
		env.settings.warmup_runs = atoi(pargs->warmup_runs);
	if (pargs->skip)
		skip = atoi(pargs->skip);

	ret = system_info_init(&sys, pargs->custom_sysinfo);
	if (ret) {
		printk(KERN_ERR "Failed acquiring system information\n");
		return -1;
	}

	if (!strcmp(pargs->storage, "sqlite3")) {
		ret = storage_init(&env.storage, &storage_sqlite3, pargs->db_path);
	} else if (!strcmp(pargs->storage, "csv")) {
		ret = storage_init(&env.storage, &storage_csv, pargs->db_path);
	} else {
		printk(KERN_ERR "No such storage backend %s\n", pargs->db_path);
		return -1;
	}

	if (ret) {
		printk(KERN_ERR "Failed storage init, %s possibly not compiled into cbenchsuite?\n",
				pargs->storage);
		goto error_storage_init;
	}

	ret = storage_add_sysinfo(&env.storage, &sys);
	if (ret) {
		printk(KERN_ERR "Failed to add systeminfo into storage\n");
		goto error_storage_sysinfo;
	}

	ret = mod_mgr_init(&mm, env.bin_dir);
	if (ret) {
		printk(KERN_ERR "Failed to initialize module manager\n");
		goto error_storage_sysinfo;
	}

	if (as_benchsuite)
		ret = execute_benchsuite_args(&mm, &env, skip, pargs, argc, argv);
	else
		ret = execute_cmd_args(&mm, &env, skip, pargs, argc, argv);
	if (ret) {
		printk(KERN_ERR "Failed executing all commandline arguments\n");
		goto error_modmgr;
	}

error_modmgr:
	mod_mgr_exit(&mm);
error_storage_sysinfo:
	storage_exit(&env.storage);
error_storage_init:
	system_info_free(&sys);

	return ret;
}

int cmd_list(struct arguments *pargs, int argc, char **argv) {
	struct mod_mgr mm;
	int arg_found = 0;
	int i;
	int ret;

	ret = mod_mgr_init(&mm, pargs->module_dir);
	if (ret) {
		printk(KERN_ERR "Failed to initialize module manager\n");
		return -1;
	}

	for (i = 1; i != argc; ++i) {
		if (!argv[i])
			continue;

		ret |= mod_mgr_list_module(&mm, argv[i], pargs->verbose);
		arg_found = 1;
	}

	if (!arg_found)
		ret |= mod_mgr_list_module(&mm, NULL, pargs->verbose);
	mod_mgr_exit(&mm);
	return ret;
}

static const char *expand_home(const char *rel_path)
{
	const char *home = getenv("HOME");
	char *path;

	if (*rel_path != '~')
		return rel_path;

	if (!home) {
		printk(KERN_ERR "HOME environment variable is not set but needed\n");
		return NULL;
	}

	path = malloc(strlen(home) + strlen(rel_path));

	if (!path)
		return NULL;

	sprintf(path, "%s%s", home, rel_path+1);

	return path;
}

static int expand_pathes(struct arguments *args)
{
	args->work_dir = expand_home(args->work_dir);
	args->download_dir = expand_home(args->download_dir);
	args->module_dir = expand_home(args->module_dir);
	args->db_path = expand_home(args->db_path);

	if (!args->work_dir || !args->download_dir || !args->module_dir
			|| !args->db_path)
		return -1;
	return 0;
}

static int create_dirs(struct arguments *args)
{
	char *buf;
	int ret;

	buf = malloc(strlen(args->download_dir) + 64);
	if (!buf)
		return -1;

	sprintf(buf, "mkdir -p %s", args->download_dir);
	ret = system(buf);
	free(buf);
	return ret;
}

int main(int argc, char **argv)
{
	struct arguments pargs = {
		.storage = "sqlite3",
		.db_path = CONFIG_DB_DIR,
		.work_dir = CONFIG_WORK_DIR,
		.module_dir = CONFIG_MODULE_DIR,
		.download_dir = CONFIG_DOWNLOAD_DIR,
		.custom_sysinfo = "",
		.std_err = CONFIG_STDERR_PERCENT,
		.verbose = 0,
	};
	int ret = 0;
	printk_set_log_level(CONFIG_PRINT_LOG_LEVEL);

	ret = args_parse(&pargs, argc, argv);
	if (ret) {
		return -1;
	}

	ret = expand_pathes(&pargs);
	if (ret) {
		return -1;
	}

	if (pargs.log_level)
		printk_set_log_level(atoi(pargs.log_level));

	if (pargs.cmd_help || argc <= 1) {
		print_help();
	} else {
		ret = create_dirs(&pargs);
		if (ret) {
			return -1;
		}

		if (pargs.cmd_list) {
			ret = cmd_list(&pargs, argc, argv);
		} else if (pargs.cmd_plugins) {
			ret = cmd_execute(&pargs, argc, argv, 0);
		} else {
			ret = cmd_execute(&pargs, argc, argv, 1);
		}
	}
	return ret;
}
