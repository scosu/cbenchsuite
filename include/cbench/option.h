#ifndef _CBENCH_OPTION_H_
#define _CBENCH_OPTION_H_

#include <cbench/data.h>

struct option {
	const char *name;
	struct value value;
};

static inline const struct option* option_get(const struct option *opts, const char *key)
{
	int i;
	for (i = 0; opts[i].value.type != VALUE_SENTINEL; ++i) {
		if (!strcmp(opts[i].name, key))
			return &opts[i];
	}
	return &opts[i];
}
static inline struct option* option_get_ind(struct option *opts, int ind)
{
	return &opts[ind];
}

static inline int32_t option_get_int32(const struct option *opts, const char *key)
{
	return option_get(opts, key)->value.v_int32;
}

static inline int64_t option_get_int64(const struct option *opts, const char *key)
{
	return option_get(opts, key)->value.v_int64;
}

static inline const char *option_get_str(const struct option *opts, const char *key)
{
	return option_get(opts, key)->value.v_str;
}

static inline void option_sha256_add(sha256_context *ctx, struct option *opts)
{
	int i;
	for (i = 0; opts[i].value.type != VALUE_SENTINEL; ++i) {
		value_sha256_add(ctx, &opts[i].value);
	}
}

struct option *option_parse(const struct option *defaults, const char *optstr);


#endif  /* _CBENCH_OPTION_H_ */
