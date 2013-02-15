#include <cbench/option.h>

#include <stdlib.h>
#include <string.h>

int option_iterator_next(struct option_iterator *iter, const char *str)
{
	iter->k_start = iter->v_end + 1;
	if (iter->k_start[0] == '\0')
		return 1;

	iter->k_end = strchr(iter->k_start, '=');
	if (!iter->k_end || iter->k_end[0] == '\0')
		return 1;
	iter->v_start = iter->k_end + 1;
	iter->v_end = strchr(iter->v_start, ':');
	if (!iter->v_end)
		iter->v_end = iter->v_start + strlen(iter->v_start);
	return 0;
}

int option_key_cmp(struct option_iterator *iter, const char *key)
{
	if (strlen(key) != iter->k_end - iter->k_start)
		return 1;
	return strncmp(iter->k_start, key, iter->k_end - iter->k_start);
}

long long option_parse_int_base(struct option_iterator *iter, long long min,
		long long max, int base)
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

long long option_parse_int(struct option_iterator *iter, long long min,
		long long max)
{
	return option_parse_int_base(iter, min, max, 10);
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