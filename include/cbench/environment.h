#ifndef _CBENCH_ENVIRONMENT_H_
#define _CBENCH_ENVIRONMENT_H_

#include <cbench/storage.h>

struct run_settings {
	int warmup_runs;
	int runtime_min;
	int runtime_max;
	int runs_min;
	int runs_max;
	double percent_stderr;
};

struct environment {
	const char *work_dir;
	const char *bin_dir;
	const char *download_dir;
	struct run_settings settings;
	struct storage storage;
};

#endif  /* _CBENCH_ENVIRONMENT_H_ */
