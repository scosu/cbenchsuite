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

#include <cbench/plugin.h>

#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uuid/uuid.h>

#include <klib/list.h>
#include <klib/printk.h>

#include <cbench/util.h>
#include <cbench/config.h>
#include <cbench/data.h>
#include <cbench/environment.h>
#include <cbench/module.h>
#include <cbench/option.h>
#include <cbench/requirement.h>
#include <cbench/sha256.h>
#include <cbench/storage.h>
#include <cbench/version.h>

struct plugin_exec;

static int received_sigstop = 0;
void plugins_sighandler(int signum)
{
	if (received_sigstop)
		exit(-1);
	received_sigstop = 1;
	printk(KERN_INFO "Received signal to shutdown. After the current execution cbenchsuite will be stopped\n");
}

enum plugin_exec_state {
	EXEC_STOP = 0,
	EXEC_WARMUP,
	EXEC_RUN,
	EXEC_UNDECIDED,
	EXEC_STDERR_NOT_REACHED,
	EXEC_FORCE_CONTINUE,
};

struct plugin_exec_env {
	int error_shutdown;
	enum plugin_exec_state state;
	int run;
	struct run_settings *settings;
	struct environment *env;
	struct plugin_exec *execs;
	int nr_plugins;

	pthread_barrier_t barrier;
	struct timespec time_started;
};

struct plugin_exec {
	pthread_t thread;
	int local_error;

	struct plugin *plug;
	struct plugin_exec_env *exec_env;
};

struct mon_data {
	pthread_t thread;
	int err;
	int stop;
	struct plugin_exec_env *exec_env;
};

void plugin_id_print_version(struct version *v, int verbose)
{
	int i;
	printf("        %s\n", v->version);

	if (v->comp_versions) {
		printf("          Components: ");
		for (i = 0; v->comp_versions[i].name != NULL; ++i) {
			struct comp_version *cv = &v->comp_versions[i];
			if (i != 0)
				printf(", ");
			printf("%s-%s", cv->name, cv->version);
		}
		printf("\n");
	}
	if (v->requirements) {
		printf("          Requirements\n");
		for (i = 0; v->requirements[i].name; ++i) {
			if (!v->requirements[i].found || verbose == 2) {
				printf("            %s (%s)\n",
						v->requirements[i].name,
						v->requirements[i].found ? "found": "not found");
			}
			if (verbose == 2 && v->requirements[i].description)
				printf("            %s\n", v->requirements[i].description);
		}
	}
	if (verbose == 2)
		printf("          Independent values: %d\n", v->nr_independent_values);
	if (v->default_options) {
		printf("          Options\n");
		for (i = 0; v->default_options[i].name != NULL; ++i) {
			struct header *o = &v->default_options[i];

			printf("            %s (Default: ", o->name);
			value_print(&o->opt_val);
			if (o->unit)
				printf("%s", o->unit);
			printf(")\n");
		}
	}
}

void plugin_id_print(const struct plugin_id *plug, int verbose)
{
	int i;
	printf("    %-20s", plug->name);
	if (plug->description)
		printf(" %s\n", plug->description);
	else
		printf("\n");
	if (verbose == 2) {
		printf("      Versions\n");
		for (i = 0; plug->versions[i].version; ++i) {
			plugin_id_print_version(&plug->versions[i], verbose);
		}
	} else if (verbose) {
		printf("      Version\n");
		plugin_id_print_version(&plug->versions[0], verbose);
	}
}

void plugin_calc_sha256(struct plugin *plug)
{
	sha256_context ctx;
	int i;
	struct comp_version *vers = plug->version->comp_versions;

	sha256_starts(&ctx);

	sha256_add_str(&ctx, plug->id->name);
	sha256_add_str(&ctx, plug->version->version);
	sha256_add(&ctx, &plug->version->nr_independent_values);

	sha256_finish_str(&ctx, plug->sha256);

	if (vers) {
		sha256_context ver_ctx;
		sha256_starts(&ver_ctx);

		for (i = 0; vers[i].name; ++i) {
			sha256_add_str(&ver_ctx, vers[i].name);
			sha256_add_str(&ver_ctx, vers[i].version);
		}

		sha256_finish_str(&ver_ctx, plug->ver_sha256);
	} else {
		plug->ver_sha256[0] = '\0';
	}

	if (plug->options) {
		sha256_context opt_ctx;

		sha256_starts(&opt_ctx);
		option_sha256_add(&opt_ctx, plug->options);
		sha256_finish_str(&opt_ctx, plug->opt_sha256);
	} else {
		plug->opt_sha256[0] = '\0';
	}
}

void plugins_calc_sha256(struct list_head *plugins, char *sha256)
{
	sha256_context ctx;
	struct plugin *plug;

	sha256_starts(&ctx);

	list_for_each_entry(plug, plugins, plugin_grp) {
		sha256_add_str(&ctx, plug->sha256);
		sha256_add_str(&ctx, plug->opt_sha256);
		sha256_add_str(&ctx, plug->ver_sha256);
	}

	sha256_finish_str(&ctx, sha256);
}

static inline void plugin_execenv_barrier(struct plugin_exec_env *env)
{
	int ret = pthread_barrier_wait(&env->barrier);
	if (ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD) {
		printk(KERN_ERR "Execution thread barrier error\n");
		env->error_shutdown = 1;
	}
}

static inline void plugin_exec_barrier(struct plugin_exec *exec)
{
	plugin_execenv_barrier(exec->exec_env);
}

static inline void plugin_exec_stop_bg(struct plugin_exec *exec)
{
	struct plugin_exec *all = exec->exec_env->execs;
	int nr_plugins = exec->exec_env->nr_plugins;
	int i;

	for (i = 0; i != nr_plugins; ++i) {
		if (&all[i] == exec)
			continue;
		if (all[i].plug->id->stop)
			all[i].plug->id->stop(all[i].plug);
	}
}

static const int EXEC_FUNC_NOTIFY_BG = 1;

static inline void plugin_exec_function(int (*func)(struct plugin *plug),
		struct plugin_exec *exec, int nr_func, int notify_bg_procs)
{
	int ret;
	if (func == NULL)
		goto barrier_only;
	if (exec->local_error)
		goto barrier_only;

	exec->plug->called_fun = nr_func;

	ret = func(exec->plug);
	if (ret) {
		exec->exec_env->error_shutdown = 1;
		exec->local_error = 1;
		printk(KERN_ERR "Failed executing function %d\n", nr_func);
	}

barrier_only:
	if (notify_bg_procs)
		plugin_exec_stop_bg(exec);

	plugin_exec_barrier(exec);

	ret = thread_set_priority(CONFIG_EXECUTION_PRIO);
	if (ret)
		printk(KERN_NOTICE "Execution thread failed to set priority %d."
				" Operating with unchanged priority.\n",
				CONFIG_EXECUTION_PRIO);

	// Between those barriers, the main program persists all gathered data
	plugin_exec_barrier(exec);
}

static int plugin_generic_stderr_check(struct plugin *plug, double std_err_percent)
{
	struct data *d;
	int i = 0;
	while (1) {
		int nr_values = 0;
		double sum = 0;
		double variance = 0;
		double mean;
		double std_err;
		double std_dev;
		double std_err_thresh;

		plugin_for_each_result(plug, d) {
			switch (d->data[i].type) {
			case VALUE_SENTINEL:
				goto finished;
			case VALUE_STRING:
				goto not_parsable;
			case VALUE_INT32:
				sum += d->data[i].v_int32;
				break;
			case VALUE_INT64:
				sum += d->data[i].v_int64;
				break;
			case VALUE_FLOAT:
				sum += d->data[i].v_flt;
				break;
			case VALUE_DOUBLE:
				sum += d->data[i].v_dbl;
				break;
			default:
				break;
			}
			++nr_values;
		}

		if (nr_values <= 1)
			return 0;

		mean = sum / nr_values;

		plugin_for_each_result(plug, d) {
			double dev = 0;
			switch (d->data[i].type) {
			case VALUE_INT32:
				dev = d->data[i].v_int32;
				break;
			case VALUE_INT64:
				dev = d->data[i].v_int64;
				break;
			case VALUE_FLOAT:
				dev = d->data[i].v_flt;
				break;
			case VALUE_DOUBLE:
				dev = d->data[i].v_dbl;
				break;
			default:
				break;
			}
			dev -= mean;
			variance += dev * dev;
		}

		//variance /= nr_values;
		std_dev = sqrt(variance);
		std_err = std_dev / sqrt(nr_values);
		std_err_thresh = mean / 100.0 * std_err_percent;

		printk(KERN_DEBUG "check_stderr: nr_values: %d mean: %f variance: %f std_dev: %f std_err: %f std_err_thresh: %f\n",
				nr_values, mean, variance, std_dev, std_err,
				std_err_thresh);

		if (std_err > std_err_thresh)
			return 0;

not_parsable:
		++i;
	}

finished:
	return 1;
}

static const int exec_funcs_before_run = 4;
static const int exec_funcs_after_run = 5;

static void *plugins_thread_execute(void *data)
{
	struct plugin_exec *exec = (struct plugin_exec *)data;
	struct plugin *plug = exec->plug;
	const struct plugin_id *id = exec->plug->id;
	int ret;

	ret = thread_set_priority(CONFIG_EXECUTION_PRIO);
	if (ret)
		printk(KERN_NOTICE "Execution thread failed to set priority %d."
				" Operating with unchanged priority.\n",
				CONFIG_EXECUTION_PRIO);

	exec->local_error = 0;

	do {
		printk(KERN_DEBUG "thread plugin %s\n", id->name);
		plugin_exec_barrier(exec);
		plugin_exec_function(id->init_pre, exec, PLUGIN_CALLED_INIT_PRE, 0);
		plugin_exec_function(id->init, exec, PLUGIN_CALLED_INIT, 0);
		plugin_exec_function(id->init_post, exec, PLUGIN_CALLED_INIT_POST, 0);
		plugin_exec_function(id->run_pre, exec, PLUGIN_CALLED_RUN_PRE, 0);
		plugin_exec_barrier(exec);
		// Here the monitor thread is started
		plugin_exec_barrier(exec);
		plugin_exec_function(id->run, exec, PLUGIN_CALLED_RUN, EXEC_FUNC_NOTIFY_BG);
		plugin_exec_function(id->run_post, exec, PLUGIN_CALLED_RUN_POST, 0);
		plugin_exec_function(id->parse_results, exec, PLUGIN_CALLED_PARSE_RESULTS, 0);
		plugin_exec_function(id->exit_pre, exec, PLUGIN_CALLED_EXIT_PRE, 0);
		plugin_exec_function(id->exit, exec, PLUGIN_CALLED_EXIT, 0);
		plugin_exec_function(id->exit_post, exec, PLUGIN_CALLED_EXIT_POST, 0);

		plugin_exec_barrier(exec);
		if (exec->exec_env->state == EXEC_UNDECIDED) {
			if (id->check_stderr) {
				ret = id->check_stderr(plug);
			} else {
				ret = plugin_generic_stderr_check(plug,
						exec->exec_env->settings->percent_stderr);
			}
			if (!ret)
				exec->exec_env->state = EXEC_STDERR_NOT_REACHED;
		}
		plugin_exec_barrier(exec);
		plugin_exec_barrier(exec);
	} while (exec->exec_env->state != EXEC_STOP);
	return NULL;
}

void *plugins_thread_install(void *data)
{
	struct plugin_exec *exec = data;
	const struct plugin_id *id = exec->plug->id;
	int ret;

	if (!id->install)
		return NULL;

	ret = id->install(exec->plug);
	if (ret) {
		printk(KERN_ERR "Execution of plugin %s function install failed: %d\n",
				id->name, ret);
		exec->exec_env->error_shutdown = 1;
		exec->local_error = 1;
	}
	return NULL;
}

void *plugins_thread_uninstall(void *data)
{
	struct plugin_exec *exec = data;
	const struct plugin_id *id = exec->plug->id;
	int ret;

	if (!id->uninstall || !exec->plug->work_dir)
		return NULL;

	ret = id->uninstall(exec->plug);
	if (ret) {
		printk(KERN_ERR "Execution of plugin %s function install failed: %d\n",
				id->name, ret);
		exec->exec_env->error_shutdown = 1;
		exec->local_error = 1;
	}
	return NULL;
}

void plugin_free_data(struct plugin *plug, struct data *data)
{
	data_put(data);
}

void plugin_add_data(struct plugin *plug, struct data *data)
{
	struct plugin_exec_env *exec_env = (struct plugin_exec_env *)
						plug->exec_data;
	data->run = exec_env->run;
	list_add_tail(&data->run_data, &plug->run_data);
}

static void plugin_exec_drop_data(struct plugin_exec *exec)
{
	struct plugin *plug = exec->plug;
	struct data *data, *ndata;
	printk(KERN_DEBUG "Drop data\n");
	list_for_each_entry_safe(data, ndata, &plug->run_data, run_data) {
		list_del(&data->run_data);
		plugin_free_data(plug, data);
	}
	list_for_each_entry_safe(data, ndata, &plug->check_err_data, run_data) {
		list_del(&data->run_data);
		plugin_free_data(plug, data);
	}
}

static void plugin_exec_persist(struct plugin_exec *exec, int persist_types)
{
	struct plugin *plug = exec->plug;
	struct data *data, *ndata;
	printk(KERN_DEBUG "Persist data\n");
	list_for_each_entry_safe(data, ndata, &plug->run_data, run_data) {
		int persist = 0;
		int ret;
		printk(KERN_DEBUG "looping\n");
		if (exec->exec_env->error_shutdown)
			goto error;

		if (DATA_TYPE_MONITOR & persist_types & data->type) {
			persist = 1;
			printk(KERN_DEBUG "Would persist monitor data\n");
		} else if (DATA_TYPE_RESULT & persist_types & data->type) {
			persist = 1;
			printk(KERN_DEBUG "Would persist result data\n");
		}
		if (!persist)
			continue;

		ret = storage_add_data(&exec->exec_env->env->storage, plug, data);
		if (ret) {
			printk(KERN_ERR "Failed persisting data\n");
			exec->exec_env->error_shutdown = 1;
		}
error:
		list_del(&data->run_data);
		if (DATA_TYPE_RESULT & persist_types & data->type) {
			list_add_tail(&data->run_data, &plug->check_err_data);
		} else {
			plugin_free_data(plug, data);
		}
	}
}

static void plugins_exec_controller(struct plugin_exec_env *exec_env,
		int nr_funcs)
{
	struct plugin_exec *execs = exec_env->execs;
	int nr_plugins = exec_env->nr_plugins;
	int i;
	for (i = 0; i != nr_funcs; ++i) {
		int j;
		plugin_execenv_barrier(exec_env);
		for (j = 0; j != nr_plugins; ++j) {
			if (exec_env->state == EXEC_WARMUP)
				plugin_exec_drop_data(&execs[j]);
			else
				plugin_exec_persist(&execs[j], DATA_TYPE_MONITOR | DATA_TYPE_RESULT);
		}
		plugin_execenv_barrier(exec_env);
	}
}

static void plugins_exec_parallel(struct plugin_exec_env *exec_env,
		void *(*func)(void *data))
{
	struct plugin_exec *execs = exec_env->execs;
	int nr_plugins = exec_env->nr_plugins;
	int i;
	int ret;

	for (i = 0; i != nr_plugins; ++i) {
		ret = pthread_create(&execs[i].thread, NULL, func, &execs[i]);
		if (ret) {
			exec_env->error_shutdown = 1;
		}
	}

	for (i = 0; i != nr_plugins; ++i) {
		ret = pthread_join(execs[i].thread, NULL);
		if (ret) {
			exec_env->error_shutdown = 1;
		}
	}
}

static void plugins_install(struct plugin_exec_env *exec_env)
{
	int i;
	int ret;
	for (i = 0; i != exec_env->nr_plugins; ++i) {
		struct plugin *plug = exec_env->execs[i].plug;
		char *buf = malloc(strlen(exec_env->env->work_dir) + 128);

		plug->work_dir = NULL;
		plug->download_dir = exec_env->env->download_dir;
		if (!buf) {
			goto error;
		}
		sprintf(buf, "mkdir -p %s/%d", exec_env->env->work_dir, i);
		ret = system(buf);
		if (ret) {
			printk(KERN_ERR "Failed to create dir with command '%s'\n",
					buf);
			free(buf);
			goto error;
		}

		sprintf(buf, "%s/%d", exec_env->env->work_dir, i);
		plug->work_dir = buf;
	}
	plugins_exec_parallel(exec_env, plugins_thread_install);
	return;
error:
	while (i--) {
		struct plugin *plug = exec_env->execs[i].plug;
		if (plug->work_dir) {
			free(plug->work_dir);
			plug->work_dir = NULL;
		}
		exec_env->error_shutdown = 1;
	}
}

static void plugins_uninstall(struct plugin_exec_env *exec_env)
{
	int i;
	int ret;
	char *tmp_cmd = malloc(strlen(exec_env->env->work_dir) + 128);
	if (!tmp_cmd) {
		exec_env->error_shutdown = 1;
		return;
	}

	plugins_exec_parallel(exec_env, plugins_thread_uninstall);

	for (i = 0; i != exec_env->nr_plugins; ++i) {
		struct plugin *plug = exec_env->execs[i].plug;

		if (plug->work_dir) {
			free(plug->work_dir);
		}

		sprintf(tmp_cmd, "rm -Rf %s/%d", exec_env->env->work_dir, i);
		ret = system(tmp_cmd);
		if (ret) {
			printk(KERN_ERR "Failed to remove dir with command '%s'\n",
					tmp_cmd);
			exec_env->error_shutdown = 1;
			continue;
		}
	}

	free(tmp_cmd);
}

static void timespec_sub(struct timespec *tgt, struct timespec *src)
{
	tgt->tv_sec -= src->tv_sec;
	if (tgt->tv_nsec < src->tv_nsec) {
		--tgt->tv_sec;
		tgt->tv_nsec = 1000000000 + tgt->tv_nsec - src->tv_nsec;
	} else {
		tgt->tv_nsec -= src->tv_nsec;
	}
}

void *plugin_thread_monitor(void *data)
{
	struct mon_data *mon = (struct mon_data*)data;
	struct plugin_exec *execs = mon->exec_env->execs;
	int nr_plugins = mon->exec_env->nr_plugins;
	int i;
	int ret;
	struct timespec now;
	struct timespec last;
	struct timespec delta;
	struct timespec to_sleep;
	struct timespec two_second = {
		.tv_sec = 2,
		.tv_nsec = 0,
	};
	ret = thread_set_priority(CONFIG_MONITOR_PRIO);
	if (ret)
		printk(KERN_NOTICE "Monitor thread failed to set priority %d."
				" Operating with unchanged priority.\n",
				CONFIG_MONITOR_PRIO);
	clock_gettime(CLOCK_MONOTONIC_RAW, &last);
	last.tv_sec -= 1;
	while (!mon->stop) {
		ret = 0;
		for (i = 0; i != nr_plugins; ++i) {
			if (!execs[i].plug->id->monitor)
				continue;
			ret |= execs[i].plug->id->monitor(execs[i].plug);
		}
		clock_gettime(CLOCK_MONOTONIC_RAW, &now);

		delta = now;
		timespec_sub(&delta, &last);
		to_sleep = two_second;
		timespec_sub(&to_sleep, &delta);

		if (to_sleep.tv_sec < two_second.tv_sec) {
			nanosleep(&to_sleep, NULL);
		}
		last = now;
	}
	return NULL;
}

int plugins_execute(struct environment *env, struct list_head *plugins)
{
	int i;
	int nr_plugins;
	int ret;
	struct plugin *plg;
	struct plugin_exec *execs;
	struct plugin_exec_env exec_env = {
		.error_shutdown = 0,
		.state = EXEC_WARMUP,
		.run = 0,
		.settings = &env->settings,
		.env = env,
		.execs = NULL,
		.nr_plugins = 0,
	};
	struct mon_data monitor = {
		.err = 0,
		.stop = 0,
		.exec_env = &exec_env,
	};
	struct run_settings *settings = exec_env.settings;
	struct timespec time_now;
	u64 time_diff;
	unsigned int min_time;
	unsigned int max_time;
	int max_ind_values = 1;
	char sha256[65];
	sighandler_t sigterm_handler;
	sighandler_t sigint_handler;

	sigterm_handler = signal(SIGTERM, plugins_sighandler);
	sigint_handler = signal(SIGINT, plugins_sighandler);

	plugins_calc_sha256(plugins, sha256);
	storage_init_plg_grp(&env->storage, plugins, sha256);

	ret = thread_set_priority(CONFIG_CONTROLLER_PRIO);
	if (ret)
		printk(KERN_NOTICE "Controller thread failed to set priority %d."
				" Operating with unchanged priority.\n",
				CONFIG_CONTROLLER_PRIO);

	i = 0;
	list_for_each_entry(plg, plugins, plugin_grp) ++i;
	nr_plugins = exec_env.nr_plugins = i;
	printk(KERN_DEBUG "nr_plugins: %d\n", nr_plugins);

	ret = pthread_barrier_init(&exec_env.barrier, NULL, nr_plugins + 1);
	if (ret) {
		printk(KERN_ERR "Failed barrier init for %d clients\n", nr_plugins);
		exec_env.error_shutdown = 1;
		goto failed_barrier_init;
	}

	execs = exec_env.execs = malloc(sizeof(*execs) * exec_env.nr_plugins);
	if (!execs) {
		printk(KERN_ERR "Out of memory\n");
		exec_env.error_shutdown = 1;
		goto failed_exec_alloc;
	}
	memset(execs, 0, sizeof(*execs) * nr_plugins);


	/*
	 * INSTALLATION
	 */

	i = 0;
	printk(KERN_INFO "Running plugins:");
	list_for_each_entry(plg, plugins, plugin_grp) {
		printk(KERN_CNT " %s.%s", plg->mod->name, plg->id->name);
		execs[i].plug = plg;
		execs[i].exec_env = &exec_env;
		plg->exec_data = &exec_env;
		if (plg->version->nr_independent_values > max_ind_values)
			max_ind_values = plg->version->nr_independent_values;

		++i;
	}
	printk(KERN_CNT "\n");
	min_time = exec_env.env->settings.runtime_min * max_ind_values;
	max_time = exec_env.env->settings.runtime_max * max_ind_values;
	printk(KERN_INFO "\tRuntime without warmup between %02uh%02u and %02uh%02u\n",
			min_time / 3600, min_time / 60,
			max_time / 3600, max_time / 60);

	printk(KERN_INFO "\tInstalling plugins\n");
	plugins_install(&exec_env);

	if (exec_env.error_shutdown) {
		printk(KERN_ERR "Failed installing plugins\n");
		goto uninstall;
	}


	/*
	 * EXECUTION
	 */
	for (i = 0; i != nr_plugins; ++i) {
		ret = pthread_create(&execs[i].thread, NULL,
				plugins_thread_execute, &execs[i]);
		if (ret) {
			exec_env.error_shutdown = 1;
		}
	}

	while (exec_env.state != EXEC_STOP && !received_sigstop) {
		char uuid[37];
		uuid_t uuid_raw;
		uuid_generate(uuid_raw);
		uuid_unparse_lower(uuid_raw, uuid);

		if (exec_env.state == EXEC_WARMUP) {
			if (exec_env.run >= settings->warmup_runs) {
				exec_env.state = EXEC_RUN;
				exec_env.run = 0;
				clock_gettime(CLOCK_MONOTONIC_RAW,
						&exec_env.time_started);
			}
		} else {
			exec_env.state = EXEC_RUN;
		}

		if (exec_env.state == EXEC_RUN) {
			printk(KERN_INFO "\tExecution run %d uuid:'%s'\n",
					exec_env.run + 1, uuid);
			storage_init_run(&env->storage, uuid,
					exec_env.run + settings->warmup_runs);
		} else {
			printk(KERN_INFO "\tWarmup run %d\n", exec_env.run + 1);
		}
		printk(KERN_DEBUG "Loop run %d state %d\n", exec_env.run,
				exec_env.state);
		plugin_execenv_barrier(&exec_env);



		plugins_exec_controller(&exec_env, exec_funcs_before_run);



		/*
		 * RUN
		 */
		plugin_execenv_barrier(&exec_env);
		ret = pthread_create(&monitor.thread, NULL, plugin_thread_monitor,
				&monitor);
		if (ret) {
			printk(KERN_ERR "Failed starting monitor thread\n");
			exec_env.error_shutdown = 1;
		}

		plugin_execenv_barrier(&exec_env);
		// Waiting for execution to finish
		plugin_execenv_barrier(&exec_env);

		monitor.stop = 1;
		ret = pthread_join(monitor.thread, NULL);
		monitor.stop = 0;
		if (ret) {
			printk(KERN_ERR "Failed monitor thread join\n");
			exec_env.error_shutdown = 1;
		}
		for (i = 0; i != nr_plugins; ++i) {
			if (exec_env.state == EXEC_WARMUP)
				plugin_exec_drop_data(&execs[i]);
			else
				plugin_exec_persist(&execs[i], DATA_TYPE_MONITOR | DATA_TYPE_RESULT);
		}
		plugin_execenv_barrier(&exec_env);
		/*
		 * END RUN
		 */



		plugins_exec_controller(&exec_env, exec_funcs_after_run);


		if (exec_env.state == EXEC_RUN) {
			storage_exit_run(&env->storage);
		}

		++exec_env.run;
		if (exec_env.error_shutdown) {
			printk(KERN_ERR "Some plugin failed to run, aborting"
					" plugin group execution\n");
			exec_env.state = EXEC_STOP;
		} else if (exec_env.state != EXEC_WARMUP) {
			exec_env.state = EXEC_UNDECIDED;
			clock_gettime(CLOCK_MONOTONIC_RAW, &time_now);
			time_diff = time_now.tv_sec - exec_env.time_started.tv_sec;
			if (time_diff < min_time) {
				printk(KERN_INFO "\t\tMinimum runtime not reached, continuing\n");
				exec_env.state = EXEC_FORCE_CONTINUE;
			} else if (time_diff > max_time) {
				printk(KERN_INFO "\t\tMaximum runtime reached, stopping\n");
				exec_env.state = EXEC_STOP;
			} else if (settings->runs_min > exec_env.run) {
				printk(KERN_INFO "\t\tMinimum number of runs (%d) not reached, continuing\n",
						settings->runs_min);
				exec_env.state = EXEC_FORCE_CONTINUE;
			} else if (settings->runs_max <= exec_env.run) {
				printk(KERN_INFO "\t\tMaximum number of runs not reached, stopping\n");
				exec_env.state = EXEC_STOP;
			}
		}
		plugin_execenv_barrier(&exec_env);
		plugin_execenv_barrier(&exec_env);
		if (exec_env.state == EXEC_UNDECIDED) { // No plugin needs to continue running
			exec_env.state = EXEC_STOP;
			printk(KERN_INFO "\t\tNecessary standard error reached, stopping\n");
		} else if (exec_env.state == EXEC_STDERR_NOT_REACHED) {
			printk(KERN_INFO "\t\tNecessary standard error not reached, continuing\n");
		}
		plugin_execenv_barrier(&exec_env);
	}

	for (i = 0; i != nr_plugins; ++i) {
		ret = pthread_join(execs[i].thread, NULL);
		if (ret) {
			printk(KERN_ERR "Failed to join runner thread %d\n", i);
			exec_env.error_shutdown = 1;
		}
	}

	for (i = 0; i != nr_plugins; ++i) {
		plugin_exec_drop_data(&execs[i]);
	}


	/*
	 * UNINSTALL
	 */

uninstall:
	printk(KERN_INFO "\tUninstalling plugins\n");
	plugins_uninstall(&exec_env);

	storage_exit_plg_grp(&env->storage);

	free(execs);
failed_exec_alloc:
failed_barrier_init:
	pthread_barrier_destroy(&exec_env.barrier);
	signal(SIGTERM, sigterm_handler);
	signal(SIGINT, sigint_handler);
	if (received_sigstop)
		return -1;
	return exec_env.error_shutdown;
}

int plugin_version_check_requirements(const struct plugin_id *plug,
		const struct version *ver)
{
	int i;
	int ret = 0;
	if (!ver->requirements)
		return 0;

	for (i = 0; ver->requirements[i].name; ++i) {
		struct requirement *req = &ver->requirements[i];
		if (req->found)
			continue;

		printk(KERN_WARNING "Plugin %s version %s has a missing requirement: %s\n",
				plug->name, ver->version, req->name);
		if (req->description)
			printk(KERN_INFO "  Requirement description: %s\n", req->description);
		ret = -1;
	}

	return ret;
}
