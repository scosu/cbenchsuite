
#include <plugin.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <klib/printk.h>

#include <environment.h>

struct plugin_exec;

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

static inline void plugin_exec_barrier(struct plugin_exec *exec)
{
	int ret = pthread_barrier_wait(&exec->exec_env->barrier);
	if (ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD) {
		exec->exec_env->error_shutdown = 1;
	}
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
		return;
	if (exec->local_error)
		return;

	ret = func(exec->plug);
	if (ret) {
		exec->exec_env->error_shutdown = 1;
		exec->local_error = 1;
		printk(KERN_ERR "Failed executing function %d\n", nr_func);
	}
	if (notify_bg_procs)
		plugin_exec_stop_bg(exec);

	plugin_exec_barrier(exec);
	// Between those barriers, the main program persists all gathered data
	plugin_exec_barrier(exec);
}

static const int exec_funcs_before_run = 4;
static const int exec_funcs_after_run = 5;

static void *plugins_thread_execute(void *data)
{
	struct plugin_exec *exec = (struct plugin_exec *)data;
	struct plugin *plug = exec->plug;
	const struct plugin_id *id = exec->plug->id;
	int ret;

	exec->local_error = 0;

	do {
		plugin_exec_barrier(exec);
		plugin_exec_function(id->init_pre, exec, 1, 0);
		plugin_exec_function(id->init, exec, 2, 0);
		plugin_exec_function(id->init_post, exec, 3, 0);
		plugin_exec_function(id->run_pre, exec, 4, 0);
		plugin_exec_function(id->run, exec, 5, EXEC_FUNC_NOTIFY_BG);
		plugin_exec_function(id->run_post, exec, 6, 0);
		plugin_exec_function(id->parse_results, exec, 7, 0);
		plugin_exec_function(id->exit_pre, exec, 8, 0);
		plugin_exec_function(id->exit, exec, 9, 0);
		plugin_exec_function(id->exit_post, exec, 10, 0);

		plugin_exec_barrier(exec);
		if (exec->exec_env->state == EXEC_UNDECIDED && id->check_stderr) {
			ret = id->check_stderr(plug);
			if (!ret)
				exec->exec_env->state = EXEC_STDERR_NOT_REACHED;
		}
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
	}
	return NULL;
}

void *plugins_thread_uninstall(void *data)
{
	struct plugin_exec *exec = data;
	const struct plugin_id *id = exec->plug->id;
	int ret;

	if (!id->uninstall)
		return NULL;

	ret = id->uninstall(exec->plug);
	if (ret) {
		printk(KERN_ERR "Execution of plugin %s function install failed: %d\n",
				id->name, ret);
	}
	return NULL;
}

static void plugin_exec_drop_data(struct plugin_exec *exec)
{
	printk(KERN_DEBUG "Drop data\n");
}

static void plugin_exec_persist(struct plugin_exec *exec)
{
	printk(KERN_DEBUG "Persist data\n");
}

static void plugins_exec_controller(struct plugin_exec_env *exec_env,
		int nr_funcs)
{
	struct plugin_exec *execs = exec_env->execs;
	int nr_plugins = exec_env->nr_plugins;
	int i;
	int ret;
	for (i = 0; i != exec_funcs_before_run; ++i) {
		int j;
		ret = pthread_barrier_wait(&exec_env->barrier);
		if (ret) {
			printk(KERN_ERR "barrier error\n");
			exec_env->error_shutdown = 1;
		}
		for (j = 0; j != nr_plugins; ++j) {
			if (exec_env->state == EXEC_WARMUP)
				plugin_exec_drop_data(&execs[j]);
			else
				plugin_exec_persist(&execs[j]);
		}
		ret = pthread_barrier_wait(&exec_env->barrier);
		if (ret) {
			printk(KERN_ERR "barrier error\n");
			exec_env->error_shutdown = 1;
		}
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
	char *tmp_cmd = malloc(strlen(exec_env->env->work_dir) + 128);
	if (!tmp_cmd) {
		exec_env->error_shutdown = 1;
		return;
	}
	for (i = 0; i != exec_env->nr_plugins; ++i) {
		sprintf(tmp_cmd, "mkdir -p %s/%d", exec_env->env->work_dir, i);
		ret = system(tmp_cmd);
		if (ret) {
			printk(KERN_ERR "Failed to create dir with command '%s'\n",
					tmp_cmd);
			exec_env->error_shutdown = 1;
			continue;
		}
	}

	plugins_exec_parallel(exec_env, plugins_thread_install);

	free(tmp_cmd);
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

	plugins_exec_parallel(exec_env, plugins_thread_install);

	for (i = 0; i != exec_env->nr_plugins; ++i) {
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

void *plugin_thread_monitor(void *data)
{
	struct mon_data *mon = (struct mon_data*)data;
	struct plugin_exec *execs = mon->exec_env->execs;
	int nr_plugins = mon->exec_env->nr_plugins;
	int i;
	int ret;
	while (!mon->stop) {
		ret = 0;
		for (i = 0; i != nr_plugins; ++i) {
			if (!execs[i].plug->id->monitor)
				continue;
			ret |= execs[i].plug->id->monitor(execs[i].plug);
		}
	}
	return NULL;
}

int plugins_execute(struct environment *env, struct plugin *plugins)
{
	int i;
	int nr_plugins;
	int ret;
	struct plugin_exec *execs;
	struct plugin_exec_env exec_env = {
		.error_shutdown = 0,
		.state = EXEC_WARMUP,
		.run = 0,
		.settings = NULL,
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

	for (i = 0; plugins[i].id != NULL; ++i);
	nr_plugins = exec_env.nr_plugins = i;

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

	for (i = 0; i != nr_plugins; ++i) {
		execs[i].plug = &plugins[i];
		execs[i].exec_env = &exec_env;
	}

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

	while (exec_env.state != EXEC_STOP) {
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

		for (i = 0; i != nr_plugins; ++i) {
			ret = pthread_create(&execs[i].thread, NULL,
					plugins_thread_uninstall, &execs[i]);
			if (ret) {
				printk(KERN_ERR "Failed to create uninstall thread %d\n", i);
				exec_env.error_shutdown = 1;
			}
		}

		pthread_barrier_wait(&exec_env.barrier);

		plugins_exec_controller(&exec_env, exec_funcs_before_run);

		// RUN method
		ret = pthread_create(&monitor.thread, NULL, plugin_thread_monitor,
				&monitor);
		if (ret) {
			printk(KERN_ERR "Failed starting monitor thread\n");
			exec_env.error_shutdown = 1;
		}

		ret = pthread_barrier_wait(&exec_env.barrier);
		if (ret) {
			printk(KERN_ERR "Failed barrier\n");
			exec_env.error_shutdown = 1;
		}

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
				plugin_exec_persist(&execs[i]);
		}

		plugins_exec_controller(&exec_env, exec_funcs_after_run);

		ret = pthread_barrier_wait(&exec_env.barrier);
		if (ret) {
			printk(KERN_ERR "Failed barrier\n");
			exec_env.error_shutdown = 1;
		}
		++exec_env.run;
		if (exec_env.state != EXEC_WARMUP) {
			clock_gettime(CLOCK_MONOTONIC_RAW, &time_now);
			time_diff = time_now.tv_sec - exec_env.time_started.tv_sec;
			exec_env.state = EXEC_UNDECIDED;
			if (settings->runs_min > exec_env.run
					|| settings->runtime_min > time_diff) {
				exec_env.state = EXEC_FORCE_CONTINUE;
			} else if (settings->runs_max <= exec_env.run
					|| settings->runtime_max <= time_diff) {
				exec_env.state = EXEC_STOP;
			}
		}
		ret = pthread_barrier_wait(&exec_env.barrier);
		if (ret) {
			printk(KERN_ERR "Failed barrier\n");
			exec_env.error_shutdown = 1;
		}
	}

	for (i = 0; i != nr_plugins; ++i) {
		ret = pthread_join(execs[i].thread, NULL);
		if (ret) {
			printk(KERN_ERR "Failed to join runner thread %d\n", i);
			exec_env.error_shutdown = 1;
		}
	}


	/*
	 * UNINSTALL
	 */

uninstall:
	plugins_uninstall(&exec_env);

	free(execs);
failed_exec_alloc:
failed_barrier_init:
	pthread_barrier_destroy(&exec_env.barrier);
	return exec_env.error_shutdown;
}

