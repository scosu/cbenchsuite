#ifndef _BENCHSUITE_H_
#define _BENCHSUITE_H_

struct plugin_link {
	char *name;
	int add_instances;
};

struct benchsuite {
	char *name;
	struct plugin_link **plugin_grps;
};

#endif  /* _BENCHSUITE_H_ */
