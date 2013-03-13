/*
 * Cbench - A C benchmarking suite for Linux benchmarking.
 * Copyright (C) 2013  Markus Pargmann <mpargmann@allfex.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your header) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <cbench/option.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cbench/data.h>
#include <cbench/util.h>

struct option_iterator {
	const char *k_start;
	const char *k_end;
	const char *v_start;
	const char *v_end;
	int end;
};

#define options_for_each_entry(opt_itr, opt_str) 			\
	for ((opt_itr)->v_end = (opt_str) - 1, (opt_itr)->end = 0;	\
		!option_iterator_next(opt_itr, opt_str);)

int option_value_strlen(struct option_iterator *iter)
{
	return iter->v_end - iter->v_start;
}

void option_value_copy(struct option_iterator *iter, char *buf)
{
	int len = option_value_strlen(iter);
	memcpy(buf, iter->v_start, option_value_strlen(iter));
	buf[len] = '\0';
}

int option_iterator_next(struct option_iterator *iter, const char *str)
{
	if (iter->end)
		return 1;
	iter->k_start = iter->v_end + 1;
	if (iter->k_start[0] == '\0')
		return 1;

	iter->k_end = strchr(iter->k_start, '=');
	if (!iter->k_end || iter->k_end[0] == '\0')
		return 1;
	iter->v_start = iter->k_end + 1;
	iter->v_end = strchr(iter->v_start, ':');
	if (!iter->v_end) {
		iter->v_end = iter->v_start + strlen(iter->v_start);
		iter->end = 1;
	}
	return 0;
}

int option_key_cmp(struct option_iterator *iter, const char *key)
{
	if (strlen(key) != iter->k_end - iter->k_start)
		return 1;
	return strncmp(iter->k_start, key, iter->k_end - iter->k_start);
}

long long option_parse_int_base(struct option_iterator *iter, int base)
{
	char buf[128];
	long long val;
	int size = iter->v_end - iter->v_start;
	if (size >= 126)
		size = 126;
	memcpy(buf, iter->v_start, size);
	buf[size] = '\0';
	val = strtoll(buf, NULL, base);
	return val;
}

long long option_parse_int(struct option_iterator *iter)
{
	return option_parse_int_base(iter, 10);
}

int option_parse_bool(struct option_iterator *iter)
{
	int v_size = iter->v_end - iter->v_start;
	if ((v_size == strlen("true") && !strncasecmp(iter->v_start, "true", v_size))
			|| !strncmp(iter->v_start, "1", v_size))
		return 1;
	if ((v_size == strlen("false") && !strncasecmp(iter->v_start, "false", v_size))
			|| strncmp(iter->v_start, "1", v_size))
		return 0;
	return 0;
}

int option_parse(const struct header *defaults, const char *optstr,
		struct header **out)
{
	struct header *opts;
	struct option_iterator itr;
	int i;
	int nr_items;

	if (!defaults) {
		*out = NULL;
		return 0;
	}

	for (i = 0; defaults[i].name != NULL; ++i) ;
	nr_items = i;

	opts = malloc(sizeof(*opts) * (nr_items + 1));
	if (!opts) {
		return -1;
	}
	memcpy(opts, defaults, sizeof(*opts) * (nr_items + 1));

	if (!optstr) {
		*out = opts;
		return 0;
	}

	options_for_each_entry(&itr, optstr) {
		for (i = 0; i != nr_items; ++i) {
			if (!option_key_cmp(&itr, opts[i].name)) {
				switch (opts[i].opt_val.type) {
				case VALUE_STRING:
					opts[i].opt_val.v_str = malloc(option_value_strlen(&itr) + 1);
					option_value_copy(&itr, opts[i].opt_val.v_str);
					break;
				case VALUE_INT32:
					opts[i].opt_val.v_int32 = option_parse_int(&itr);
					break;
				case VALUE_INT64:
					opts[i].opt_val.v_int64 = option_parse_int(&itr);
					break;
				default:
					printf("ERROR: Option values may not be something else than string or int (%s)\n",
							opts[i].name);
					break;
				}
				goto found;
			}
		}
		printf("ERROR: Could not find header starting at %s\n",
				itr.k_start);
		return -1;
found:
		continue;
	}
	*out = opts;
	return 0;
}

int option_to_hdr_csv(const struct header *opts, char **buf, size_t *buf_size,
		enum value_quote_type quotes)
{
	size_t est_size = 0;
	int i;
	int ret;
	char *ptr;

	for (i = 0; opts[i].name != NULL; ++i) {
		est_size += strlen(opts[i].name) + 10;
	}

	ret = mem_grow((void**)buf, buf_size, est_size);
	if (ret)
		return -1;

	ptr = *buf;
	for (i = 0; opts[i].name != NULL; ++i) {
		if (i != 0) {
			*ptr = ',';
			++ptr;
		}

		switch(quotes) {
		case QUOTE_SINGLE:
			ptr += sprintf(ptr, "'%s'", opts[i].name);
			break;
		case QUOTE_DOUBLE:
			ptr += sprintf(ptr, "\"%s\"", opts[i].name);
			break;
		case QUOTE_NONE:
			ptr += sprintf(ptr, "%s", opts[i].name);
			break;
		}
	}
	return 0;
}

int option_to_data_csv(const struct header *opts, char **buf, size_t *buf_size,
		enum value_quote_type quotes)
{
	size_t est_size = 0;
	int i;
	int ret;
	char *ptr;

	for (i = 0; opts[i].name != NULL; ++i) {
		est_size += value_as_str_len(&opts[i].opt_val);
	}

	ret = mem_grow((void**)buf, buf_size, est_size);
	if (ret)
		return -1;

	ptr = *buf;
	*ptr = '\0';
	for (i = 0; opts[i].name != NULL; ++i) {
		if (i != 0) {
			*ptr = ',';
			++ptr;
		}
		switch (opts[i].opt_val.type) {
		case VALUE_INT32:
			ptr += sprintf(ptr, "%" PRId32, opts[i].opt_val.v_int32);
			break;
		case VALUE_INT64:
			ptr += sprintf(ptr, "%" PRId64, opts[i].opt_val.v_int64);
			break;
		case VALUE_FLOAT:
			ptr += sprintf(ptr, "%f", opts[i].opt_val.v_flt);
			break;
		case VALUE_DOUBLE:
			ptr += sprintf(ptr, "%f", opts[i].opt_val.v_dbl);
			break;
		case VALUE_STRING:
			if (!opts[i].opt_val.v_str)
				break;
			switch(quotes) {
			case QUOTE_SINGLE:
				ptr += sprintf(ptr, "'%s'", opts[i].opt_val.v_str);
				break;
			case QUOTE_DOUBLE:
				ptr += sprintf(ptr, "\"%s\"", opts[i].opt_val.v_str);
				break;
			case QUOTE_NONE:
				ptr += sprintf(ptr, "%s", opts[i].opt_val.v_str);
				break;
			}
			break;
		default:
			break;
		}
	}
	return 0;
}
