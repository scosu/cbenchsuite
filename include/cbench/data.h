#ifndef _CBENCH_DATA_H_
#define _CBENCH_DATA_H_

#include <klib/list.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <cbench/sha256.h>

enum value_type {
	// Make VALUE_STRING as 0 for easier static header decleration
	VALUE_STRING = 0,
	VALUE_INT32,
	VALUE_INT64,
	VALUE_FLOAT,
	VALUE_DOUBLE,
	VALUE_SENTINEL,
};

struct value {
	enum value_type type;
	union {
		int32_t v_int32;
		int64_t v_int64;
		float v_flt;
		double v_dbl;
		char *v_str;
	};
};

enum data_value_cmp {
	DATA_UNDEFINED = 0,
	DATA_MORE_IS_BETTER,
	DATA_LESS_IS_BETTER,
};

struct header {
	const char *name;
	const char *description;
	const char *unit;
	union {
		enum data_value_cmp data_type;
		struct value opt_val;
	};
};


enum data_type {
	DATA_TYPE_MONITOR = 0x1,
	DATA_TYPE_RESULT = 0x2,
	DATA_TYPE_OTHER = 0x4,
};

struct data {
	enum data_type type;
	unsigned int run;
	struct value *data;
	struct list_head run_data;
	unsigned int cur_ind;
};

#define VALUE_STATIC_SENTINEL { .type = VALUE_SENTINEL, .v_int64 = 0 }

static inline struct data *data_alloc(enum data_type type, int nr_fields)
{
	size_t length = sizeof(struct data)
			+ sizeof(struct value) * (nr_fields + 1);
	struct data *dat = malloc(length);

	if (!dat)
		return NULL;
	memset(dat, 0, length);
	dat->type = type;
	dat->data = ((void*)dat) + sizeof(*dat);
	INIT_LIST_HEAD(&dat->run_data);
	dat->data[nr_fields].type = VALUE_SENTINEL;
	dat->cur_ind = 0;
	return dat;
}

static inline void data_put(struct data *data)
{
	int i;
	struct value *vals = data->data;

	for (i = 0; vals[i].type != VALUE_SENTINEL; ++i) {
		if (vals[i].type == VALUE_STRING && vals[i].v_str)
			free(vals[i].v_str);
	}
	free(data);
}

static inline void data_set_int32(struct data *d, int index, int32_t value)
{
	d->data[index].type = VALUE_INT32;
	d->data[index].v_int32 = value;
}
static inline void data_set_int64(struct data *d, int index, int64_t value)
{
	d->data[index].type = VALUE_INT64;
	d->data[index].v_int64 = value;
}
static inline void data_set_float(struct data *d, int index, float value)
{
	d->data[index].type = VALUE_FLOAT;
	d->data[index].v_flt = value;
}
static inline void data_set_double(struct data *d, int index, double value)
{
	d->data[index].type = VALUE_DOUBLE;
	d->data[index].v_dbl = value;
}
static inline void data_set_str(struct data *d, int index, const char *value)
{
	d->data[index].type = VALUE_STRING;
	if (value == NULL) {
		d->data[index].v_str = malloc(1);
		d->data[index].v_str[0] = '\0';
	} else {
		d->data[index].v_str = malloc(strlen(value) + 1);
		strcpy(d->data[index].v_str, value);
	}
}

static inline void data_add_int32(struct data *d, int32_t value)
{
	data_set_int32(d, d->cur_ind++, value);
}
static inline void data_add_int64(struct data *d, int64_t value)
{
	data_set_int64(d, d->cur_ind++, value);
}
static inline void data_add_float(struct data *d, float value)
{
	data_set_float(d, d->cur_ind++, value);
}
static inline void data_add_double(struct data *d, double value)
{
	data_set_double(d, d->cur_ind++, value);
}
static inline void data_add_str(struct data *d, const char *value)
{
	data_set_str(d, d->cur_ind++, value);
}

static inline int32_t data_get_int32(struct data *d, int index)
{
	return d->data[index].v_int32;
}
static inline int64_t data_get_int64(struct data *d, int index)
{
	return d->data[index].v_int64;
}
static inline float data_get_float(struct data *d, int index)
{
	return d->data[index].v_flt;
}
static inline double data_get_double(struct data *d, int index)
{
	return d->data[index].v_dbl;
}
static inline const char *data_get_str(struct data *d, int index)
{
	return d->data[index].v_str;
}

size_t values_as_str_len(const struct value *v);

enum value_quote_type {
	QUOTE_SINGLE,
	QUOTE_DOUBLE,
	QUOTE_NONE,
};

int values_to_csv(const struct value *values, char **buf, size_t *buf_len,
		enum value_quote_type quotes);

static inline int data_to_csv(const struct data *data, char **buf, size_t *buf_len,
		enum value_quote_type quotes)
{
	return values_to_csv(data->data, buf, buf_len, quotes);
}

int header_to_csv(const struct header *hdr, char **buf, size_t *buf_len,
		enum value_quote_type quotes);

static inline size_t value_as_str_len(const struct value *v)
{
	switch (v->type) {
	case VALUE_INT32:
		return 16;
	case VALUE_INT64:
		return 32;
	case VALUE_FLOAT:
		return 32;
	case VALUE_DOUBLE:
		return 64;
	case VALUE_STRING:
		if (!v->v_str)
			return 0;
		return strlen(v->v_str) + 10;
	default:
		return 0;
	}
}

void value_print(const struct value *v);

int values_nr_items(struct value *val);

int data_nr_items(struct data *data);

void value_sha256_add(sha256_context *ctx, struct value *val);

#endif  /* _CBENCH_DATA_H_ */
