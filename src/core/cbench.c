
#include <stdlib.h>

#include <klib/printk.h>

#include <cbench/core/module_manager.h>

#include <cbench/config.h>
#include <cbench/benchsuite.h>
#include <cbench/environment.h>
#include <cbench/plugin.h>

int main(void)
{
	struct mod_mgr mm;
	const char *vers[] = {NULL};
	struct benchsuite *suite;
	struct environment env = {
		.work_dir = CONFIG_WORK_PATH,
		.bin_dir = "/home/scosu/projects/cbench/build/modules/",
		.settings = {
			.warmup_runs = CONFIG_WARMUP_RUNS,
			.runs_min = CONFIG_MIN_RUNS,
			.runs_max = CONFIG_MAX_RUNS,
			.runtime_min = CONFIG_MIN_RUNTIME,
			.runtime_max = CONFIG_MAX_RUNTIME,
		},
	};
	env.settings.percent_stderr = atof(CONFIG_STDERR_PERCENT);
	printk_set_log_level(CONFIG_PRINT_LOG_LEVEL);

	mod_mgr_init(&mm, env.bin_dir);

	suite = mod_mgr_benchsuite_create(&mm, "example.suite", vers);

	benchsuite_execute(&mm, &env, suite);

	return 0;
}