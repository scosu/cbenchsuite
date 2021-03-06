#ifndef _CBENCH_BENCHSUITE_H_
#define _CBENCH_BENCHSUITE_H_

#include <cbench/version.h>

struct environment;
struct mod_mgr;

struct plugin_link {
	const char *name;
	const char *options;
	const char **version_rules;
};

struct benchsuite_id {
	const char *name;
	const char *description;
	struct version version;
	struct plugin_link **plugin_grps;
};

struct benchsuite {
	const struct benchsuite_id *id;
	struct module *mod;
};

int benchsuite_execute(struct mod_mgr *mm, struct environment *env,
		struct benchsuite *suite, int *skip);

void benchsuite_id_print(const struct benchsuite_id *suite, int verbose);

#endif  /* _CBENCH_BENCHSUITE_H_ */
