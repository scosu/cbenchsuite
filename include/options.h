#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <string.h>

struct option_iterator {
	const char *k_start;
	const char *k_end;
	const char *v_start;
	const char *v_end;
};

int option_iterator_next(struct option_iterator *iter, const char *str);

#define options_for_each_entry(opt_itr, opt_str) 			\
	for ((opt_itr)->v_end = (opt_str) - 1; 				\
		!option_iterator_next(opt_itr, opt_str);)

int option_key_cmp(struct option_iterator *iter, const char *key);

long long option_parse_int_base(struct option_iterator *iter, long long min,
		long long max, int base);

long long option_parse_int(struct option_iterator *iter, long long min,
		long long max);

int option_parse_bool(struct option_iterator *iter);

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
