#ifndef _VERSION_H_
#define _VERSION_H_


struct comp_version {
	char *name;
	char *version;
};

struct version {
	char *version;
	struct comp_version *comp_versions;
	void *data;
};

int version_compare(const char *rule, const char *ver, int rule_compare);
int version_matching(const struct version *ver, const char **rules);

#endif  /* _VERSION_H_ */
