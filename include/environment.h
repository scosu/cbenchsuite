#ifndef _ENVIRONMENT_H_
#define _ENVIRONMENT_H_

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
	struct run_settings settings;
};

#endif  /* _ENVIRONMENT_H_ */
