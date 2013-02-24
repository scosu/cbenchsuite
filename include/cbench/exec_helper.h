#ifndef _INCLUDE_CBENCH_EXEC_HELPER_H_
#define _INCLUDE_CBENCH_EXEC_HELPER_H_

#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cbench/plugin.h>

static inline int subproc_call(const char *bin, const char **argv, const char **env)
{
	int status
	pid_t pid;

	pid = vfork();

	if (pid == -1)
		return -1;
	if (pid == 0) {
		execve(bin, argv, env);
		_exit(-1);
	} else {
		int ret;
		ret = waitpid(pid, &status, 0);
		if (ret) {
			return -1;
		}
	}
	return status;
}

static inline int cbench_download(struct plugin *plug, const char *address, const char *out_name)
{
	const char *argv[] = {"-O", out_name, address, NULL};
	const char *out_dir = plugin_get_download_dir(plug);
	const char *out_buf;

	if (!access(out_name, F_OK | R_OK))
		return 0;
	out_buf = malloc(strlen(out_name) + strlen(out_dir) + 2);
	sprintf(out_buf, "%s/%s", out_dir, out_name);
	argv[1] = out_buf;

	return subproc_call("wget", argv, NULL);
}

#endif  /* _INCLUDE_CBENCH_EXEC_HELPER_H_ */
