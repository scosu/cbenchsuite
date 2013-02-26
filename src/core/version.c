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

#include <cbench/version.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <klib/printk.h>

#include <cbench/util.h>

int version_compare(const char *rule, const char *ver, int rule_compare)
{
	const char *rule_start = rule;
	const char *rule_end;
	const char *ver_start = ver;
	const char *ver_end;

	printk(KERN_DEBUG "comparing rule version %s with %s\n", rule, ver);

	while (1) {
		long long rule_int;
		long long ver_int;
		char *endptr;
		rule_int = strtoll(rule_start, &endptr, 10);
		rule_end = endptr;
		ver_int = strtoll(ver_start, &endptr, 10);
		ver_end = endptr;

		printk(KERN_DEBUG "remaining comparison: %s == %s\n",
				rule_start, ver_start);


		/*
		 * String comparison mode
		 */
		if ((*rule_end != '\0' && *rule_end != '.')
				|| (*ver_end != '\0' && *ver_end != '.')) {
			int ret;
			rule_end = strchr(rule_start, '.');
			if (!rule_end)
				rule_end = rule_start + strlen(rule_start);

			ver_end = strchr(ver_start, '.');
			if (!ver_end)
				ver_end = ver_start + strlen(ver_start);

			if (rule_end - rule_start < ver_end - ver_start)
				return -1;
			if (rule_end - rule_start > ver_end - ver_start)
				return 1;

			ret = strncmp(rule_start, ver_start, rule_end - rule_start);
			if (ret)
				return ret;
		} else {// Integer comparison mode
			if (rule_int < ver_int)
				return -1;
			if (rule_int > ver_int)
				return 1;
		}

		if (*rule_end == '\0') {
			if (*ver_end == '\0' || rule_compare)
				return 0;
		} else if (*ver_end == '\0') {
			return 1;
		}

		rule_start = rule_end + 1;
		ver_start = ver_end + 1;

		if (*rule_start == '\0') {
			if (*ver_start == '\0' || rule_compare)
				return 0;
			return -1;
		} else if (*ver_start == '\0') {
			return 1;
		}
	}
	return 0;
}

int version_matching(const struct version *ver, const char **rules)
{
	int i;
	int ret;
	if (!rules)
		return 1;
	for (i = 0; rules[i] != NULL; ++i) {
		int j;
		const char *lt = strchr(rules[i], '<');
		const char *le = strstr(rules[i], "<=");
		const char *eq = strchr(rules[i], '=');
		const char *gt = strchr(rules[i], '>');
		const char *ge = strstr(rules[i], ">=");
		const char *neq = strstr(rules[i], "!=");
		const char *id_end;
		const char *ver_start;

		if (le) {
			id_end = le;
			ver_start = id_end + 2;
		} else if (ge) {
			id_end = ge;
			ver_start = id_end + 2;
		} else if (neq) {
			id_end = neq;
			ver_start = id_end + 2;
		} else if (lt) {
			id_end = lt;
			ver_start = id_end + 1;
		} else if (eq) {
			id_end = eq;
			ver_start = id_end + 1;
		} else if (gt) {
			id_end = gt;
			ver_start = id_end + 1;
		}

		if (rules[i] == id_end) {
			printk(KERN_DEBUG "Version compare, main version check\n");
			ret = version_compare(ver_start, ver->version, 1);
			printk(KERN_DEBUG "Version comparison result: %d\n", ret);
			if (le || ge || neq) {
				if (le && ret < 0)
					return 0;
				if (ge && ret > 0)
					return 0;
				if (neq && ret == 0)
					return 0;
			} else {
				if (lt && ret <= 0)
					return 0;
				if (eq && ret != 0)
					return 0;
				if (gt && ret >= 0)
					return 0;
			}
			continue;
		}

		if (!ver->comp_versions)
			continue;

		for (j = 0; ver->comp_versions[j].name != NULL; ++j) {
			if (!strncmp(rules[i], ver->comp_versions[j].name,
						id_end - rules[i]))
				continue;
			ret = version_compare(ver_start,
					ver->comp_versions[j].version, 1);
			if (le || ge || neq) {
				if (le && ret < 0)
					return 0;
				if (ge && ret > 0)
					return 0;
				if (neq && ret == 0)
					return 0;
			} else {
				if (lt && ret <= 0)
					return 0;
				if (eq && ret != 0)
					return 0;
				if (gt && ret >= 0)
					return 0;
			}
		}
	}
	printk(KERN_DEBUG "Version match\n");
	return 1;
}

int comp_versions_to_csv(const struct comp_version *vers, char **buf,
		size_t *buf_len, enum value_quote_type quotes)
{
	size_t est_size = 0;
	int i;
	int ret;
	char *ptr;

	for (i = 0; vers[i].name != NULL; ++i) {
		est_size += 3 + strlen(vers[i].name);
	}

	ret = mem_grow((void**)buf, buf_len, est_size + 3);
	if (ret)
		return -1;

	ptr = *buf;

	for (i = 0; vers[i].name != NULL; ++i) {
		if (i != 0) {
			*ptr = ',';
			++ptr;
		}

		switch(quotes) {
		case QUOTE_SINGLE:
			ptr += sprintf(ptr, "'%s'", vers[i].name);
			break;
		case QUOTE_DOUBLE:
			ptr += sprintf(ptr, "\"%s\"", vers[i].name);
			break;
		case QUOTE_NONE:
			ptr += sprintf(ptr, "%s", vers[i].name);
			break;
		}
	}
	return 0;
}

int comp_versions_to_data_csv(const struct comp_version *vers, char **buf,
		size_t *buf_len, enum value_quote_type quotes)
{
	size_t est_size = 0;
	int i;
	int ret;
	char *ptr;

	for (i = 0; vers[i].name != NULL; ++i) {
		est_size += 3 + strlen(vers[i].version);
	}

	ret = mem_grow((void**)buf, buf_len, est_size + 3);
	if (ret)
		return -1;

	ptr = *buf;

	for (i = 0; vers[i].name != NULL; ++i) {
		if (i != 0) {
			*ptr = ',';
			++ptr;
		}

		switch(quotes) {
		case QUOTE_SINGLE:
			ptr += sprintf(ptr, "'%s'", vers[i].version);
			break;
		case QUOTE_DOUBLE:
			ptr += sprintf(ptr, "\"%s\"", vers[i].version);
			break;
		case QUOTE_NONE:
			ptr += sprintf(ptr, "%s", vers[i].version);
			break;
		}
	}
	return 0;
}
