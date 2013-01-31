#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <string.h>

struct option_iterator {
	const char *k_start;
	const char *k_end;
	const char *v_start;
	const char *v_end;
};

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

#define options_for_each_entry(opt_itr, opt_str) 			\
	for ((opt_itr)->v_end = (opt_str) - 1; 				\
		!option_iterator_next(opt_itr, opt_str);)

int option_key_cmp(struct option_iterator *iter, const char *key)
{
	if (strlen(key) != iter->v_end - iter->v_start)
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
			|| strncmp(iter->v_start, "1", v_size))
		return 1;
	if ((v_size == strlen("false") && !strncasecmp(iter->v_start, "false", v_size))
			|| strncmp(iter->v_start, "1", v_size))
		return 0;
	return 0;
}

#define option_value_length(iter) ((iter)->v_end - (iter)->v_start)

#define option_value_copy(iter, buf, max_size) 				\
	do { 								\
		int size = (max_size) - 1; 				\
		if (size > (iter)->v_end - (iter)->v_start) 		\
			size = (iter)->v_end - (iter)->v_start; 	\
		memcpy(buf, (iter)->v_start, size); 			\
		buf[size + 1] = '\0'; 					\
	} while (0)


#endif  /* _OPTIONS_H_ */
