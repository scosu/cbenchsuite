#ifndef _CBENCH_OPTION_H_
#define _CBENCH_OPTION_H_

#include <cbench/data.h>

static inline const struct header* option_get(const struct header *opts, const char *key)
{
	int i;
	for (i = 0; opts[i].name != NULL; ++i) {
		if (!strcmp(opts[i].name, key))
			return &opts[i];
	}
	return &opts[i];
}
static inline struct header* option_get_ind(struct header *opts, int ind)
{
	return &opts[ind];
}

static inline int32_t option_get_int32(const struct header *opts, const char *key)
{
	return option_get(opts, key)->opt_val.v_int32;
}

static inline int64_t option_get_int64(const struct header *opts, const char *key)
{
	return option_get(opts, key)->opt_val.v_int64;
}

static inline const char *option_get_str(const struct header *opts, const char *key)
{
	return option_get(opts, key)->opt_val.v_str;
}

static inline void option_sha256_add(sha256_context *ctx, struct header *opts)
{
	int i;
	for (i = 0; opts[i].name != NULL; ++i) {
		value_sha256_add(ctx, &opts[i].opt_val);
	}
}

int option_parse(const struct header *defaults, const char *optstr, struct header **out);

int option_to_data_csv(const struct header *opts, char **buf, size_t *buf_size,
		enum value_quote_type quotes);

int option_to_hdr_csv(const struct header *opts, char **buf, size_t *buf_size,
		enum value_quote_type quotes);

#endif  /* _CBENCH_OPTION_H_ */
