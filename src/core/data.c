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

#include <cbench/data.h>

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <cbench/util.h>

size_t value_as_str_len(const struct value *v)
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
		return strlen(v->v_str);
	default:
		return 0;
	}
}

size_t values_as_str_len(const struct value *v)
{
	int i;
	size_t ret = 0;

	for (i = 0; v[i].type != VALUE_SENTINEL; ++i)
		ret += value_as_str_len(&v[i]);
	return ret;
}

int values_to_csv(const struct value *values, char **buf, size_t *buf_len,
		enum value_quote_type quotes)
{
	size_t est_size = values_as_str_len(values);
	int i;
	int nr_items;
	int ret;
	char *ptr;

	for (i = 0; values[i].type != VALUE_SENTINEL; ++i) ;
	nr_items = i;

	est_size += nr_items * 3 + 1;
	ret = mem_grow((void**)buf, buf_len, est_size);
	if (ret)
		return ret;

	ptr = *buf;

	for (i = 0; i != nr_items; ++i) {
		if (i != 0) {
			strcat(ptr, ",");
			++ptr;
		}

		switch (values[i].type) {
		case VALUE_INT32:
			ptr += sprintf(ptr, "%" PRId32, values[i].v_int32);
			break;
		case VALUE_INT64:
			ptr += sprintf(ptr, "%" PRId64, values[i].v_int64);
			break;
		case VALUE_FLOAT:
			ptr += sprintf(ptr, "%f", values[i].v_flt);
			break;
		case VALUE_DOUBLE:
			ptr += sprintf(ptr, "%f", values[i].v_dbl);
			break;
		case VALUE_STRING:
			if (!values[i].v_str)
				break;
			switch(quotes) {
			case QUOTE_SINGLE:
				ptr += sprintf(ptr, "'%s'", values[i].v_str);
				break;
			case QUOTE_DOUBLE:
				ptr += sprintf(ptr, "\"%s\"", values[i].v_str);
				break;
			case QUOTE_NONE:
				ptr += sprintf(ptr, "%s", values[i].v_str);
				break;
			}
			break;
		default:
			break;
		}
	}
	return 0;
}

int values_nr_items(struct value *val)
{
	int i;

	for (i = 0; val[i].type != VALUE_SENTINEL; ++i) ;

	return i;
}

int data_nr_items(struct data *data)
{
	return values_nr_items(data->data);
}

void value_sha256_add(sha256_context *ctx, struct value *val)
{
	switch (val->type) {
	case VALUE_STRING:
		sha256_add_str(ctx, val->v_str);
		break;
	case VALUE_INT32:
		sha256_add(ctx, &val->v_int32);
		break;
	case VALUE_INT64:
		sha256_add(ctx, &val->v_int64);
		break;
	case VALUE_FLOAT:
		sha256_add(ctx, &val->v_flt);
		break;
	case VALUE_DOUBLE:
		sha256_add(ctx, &val->v_dbl);
		break;
	default:
		break;
	}
}

void value_print(const struct value *v)
{
	switch (v->type) {
	case VALUE_STRING:
		fputs(v->v_str, stdout);
		break;
	case VALUE_INT32:
		printf("%" PRId32, v->v_int32);
		break;
	case VALUE_INT64:
		printf("%" PRId64, v->v_int64);
		break;
	case VALUE_FLOAT:
		printf("%f", v->v_flt);
		break;
	case VALUE_DOUBLE:
		printf("%f", v->v_dbl);
		break;
	default:
		printf("invalid");
	}
}
