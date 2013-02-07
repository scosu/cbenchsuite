#ifndef _CBENCH_VERSION_H_
#define _CBENCH_VERSION_H_

struct requirement;

struct comp_version {
	char *name;
	char *version;
};

struct version {
	char *version;
	struct comp_version *comp_versions;
	void *data;

	struct requirement *requirements;

	char **data_field_names;

	int nr_independent_values;
};

int version_compare(const char *rule, const char *ver, int rule_compare);
int version_matching(const struct version *ver, const char **rules);

#endif  /* _CBENCH_VERSION_H_ */
