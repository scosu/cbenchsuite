#ifndef _CBENCH_UTIL_H_
#define _CBENCH_UTIL_H_

#include <stddef.h>
#include <stdlib.h>

int thread_set_priority(int prio);

char *file_read_complete(const char *path);

int mem_grow(void **ptr, size_t *len, size_t req_len);

#define strcmpb(conststr, buf) strncmp(conststr, buf, strlen(conststr))

void str_strip(char *buf);

#endif  /* _CBENCH_UTIL_H_ */
