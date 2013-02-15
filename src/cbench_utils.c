#include <cbench/util.h>

#include <sched.h>
#include <sys/resource.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int thread_set_priority(int prio)
{
	pid_t tid = syscall(SYS_gettid);

	return setpriority(PRIO_PROCESS, tid, prio);
}

char *file_read_complete(const char *path)
{
	FILE *f;
	int length;
	int ret;
	char *result;

	f = fopen(path, "r");
	if (!f)
		return NULL;

	ret = fseek(f, 0, SEEK_END);
	if (ret)
		return NULL;
	length = ftell(f);
	if (length < 0)
		return NULL;

	rewind(f);

	result = malloc(length);
	ret = fread(result, 1, length, f);
	if (ret <= 0) {
		free(result);
		return NULL;
	}

	return result;
}

int mem_grow(void **ptr, size_t *len, size_t req_len) {
	if (!*ptr || *len < req_len) {
		*ptr = malloc(req_len);
		if (!*ptr)
			return -1;
		*len = req_len;
	}
	return 0;
}

void str_strip(char *buf)
{
	int i;
	int j;

	for (i = 0; buf[i] != '\0' &&
			(buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n');
			++i);

	for (j = strlen(buf) - 1; j != i &&
			(buf[j] == ' ' || buf[j] == '\t' || buf[j] == '\n');
			--j);
	if (i != 0) {
		int n;
		for (n = 0; i <= j; ++n, ++i)
			buf[n] = buf[i];
	}
	buf[j+1] = '\0';
}
