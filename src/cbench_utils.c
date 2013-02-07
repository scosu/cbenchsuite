#include <cbench/util.h>

#include <sched.h>
#include <sys/resource.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>

int thread_set_priority(int prio)
{
	pid_t tid = syscall(SYS_gettid);

	return setpriority(PRIO_PROCESS, tid, prio);
}

