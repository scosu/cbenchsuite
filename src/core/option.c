/*
 * Cbench - A C benchmarking suite for Linux benchmarking.
 * Copyright (C) 2013  Markus Pargmann <mpargmann@allfex.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	if (size >= 127)
		size = 127;
	memcpy(buf, iter->v_start, size);
	buf[size + 1] = '\0';
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

struct option *option_parse(const struct option *defaults, const char *optstr)
{
	struct option *opts;
	struct option_iterator itr;
	int i;
	int nr_items;

	if (!defaults) {
		opts = malloc(sizeof(*opts));
		if (!opts)
			return NULL;
		opts->value.type = VALUE_SENTINEL;
		return opts;
	}

	for (i = 0; defaults[i].value.type != VALUE_SENTINEL; ++i) ;
	nr_items = i;

	opts = malloc(sizeof(*opts) * (nr_items + 1));
	if (!opts) {
		return NULL;
	}
	memcpy(opts, defaults, sizeof(*opts) * (nr_items + 1));

	options_for_each_entry(&itr, optstr) {
		for (i = 0; i != nr_items; ++i) {
			if (!option_key_cmp(&itr, opts[i].name)) {
				switch (opts[i].value.type) {
				case VALUE_STRING:
					opts[i].value.v_str = malloc(option_value_strlen(&itr) + 1);
					option_value_copy(&itr, opts[i].value.v_str);
					break;
				case VALUE_INT32:
					opts[i].value.v_int32 = option_parse_int(&itr);
					break;
				case VALUE_INT64:
					opts[i].value.v_int64 = option_parse_int(&itr);
					break;
				default:
					printf("ERROR: Option values may not be something else than string or int (%s)\n",
							opts[i].name);
					break;
				}
				goto found;
			}
		}
		printf("ERROR: Could not find option starting at %s\n",
				itr.k_start);
		return NULL;
found:
		continue;
	}
	return opts;
}
