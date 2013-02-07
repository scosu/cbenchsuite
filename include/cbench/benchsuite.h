#ifndef _CBENCH_BENCHSUITE_H_
#define _CBENCH_BENCHSUITE_H_

#include <cbench/version.h>

struct environment;
struct mod_mgr;

struct plugin_link {
	const char *name;
	const char *options;
	const char **version_rules;
	int add_instances;
};

struct benchsuite_id {
	const char *name;
	struct version version;
	struct plugin_link **plugin_grps;
};

struct benchsuite {
	const struct benchsuite_id *id;
	struct module *mod;
};

int benchsuite_execute(struct mod_mgr *mm, struct environment *env,
		struct benchsuite *suite);


#endif  /* _CBENCH_BENCHSUITE_H_ */
