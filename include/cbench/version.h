#ifndef _CBENCH_VERSION_H_
#define _CBENCH_VERSION_H_

#include <cbench/data.h>

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

	int nr_independent_values;

	struct header *default_options;
};

int version_compare(const char *rule, const char *ver, int rule_compare);
int version_matching(const struct version *ver, const char **rules);
int comp_versions_to_csv(const struct comp_version *vers, char **buf,
		size_t *buf_len, enum value_quote_type quotes);
int comp_versions_to_data_csv(const struct comp_version *vers, char **buf,
		size_t *buf_len, enum value_quote_type quotes);

#endif  /* _CBENCH_VERSION_H_ */
