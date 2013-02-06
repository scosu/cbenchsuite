

#include <klib/printk.h>

#include <core/module_manager.h>

#include <config.h>
#include <benchsuite.h>
#include <environment.h>
#include <plugin.h>

int main(void)
{
	struct mod_mgr mm;
	const char *vers[] = {NULL};
	struct benchsuite *suite;
	struct environment env = {
		.work_dir = "/tmp/cbench/",
		.bin_dir = "/home/scosu/projects/cbench/build/modules/",
		.settings = {
			.warmup_runs = 1,
			.runs_min = 5,
			.runs_max = 10,
		},
	};
	printk_set_log_level(CONFIG_PRINT_LOG_LEVEL);

	mod_mgr_init(&mm, env.bin_dir);

	suite = mod_mgr_benchsuite_create(&mm, "example.suite", vers);

	benchsuite_execute(&mm, &env, suite);

	return 0;
}
