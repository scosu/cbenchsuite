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

static inline int cbench_wget(struct plugin *plug, const char *address, const char *out_name)
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

static inline int cbench_download_and_untar(struct plugin *plug,
		const char *address, const char *download_name,
		const char *out_dir)
{
	const char *tar_argv[] = {"-xf", download_name, out_dir, NULL};
	const char *mkdir_argv[] = {"-p", out_dir, NULL};
	const char *plug_work_dir = plugin_get_work_dir(plug);
	const char *plug_down_dir = plugin_get_download_dir(plug);
	const char *tmp_outdir;
	const char *tmp_downdir;
	int ret;
 	tmp_outdir = malloc(strlen(plug_work_dir) + strlen(out_dir) + 2);
	if (!tmp_outdir)
		return -1;

	tmp_downdir = malloc(strlen(plug_down_dir) + strlen(download_name) + 2);
	if (!tmp_downdir) {
		ret = -1;
		goto error_alloc_downdir;
	}

	ret = cbench_wget(plug, address, download_name);
	if (ret)
		goto error_wget;

	sprintf(tmp_outdir, "%s/%s", plug_work_dir, out_dir);
	mkdir_argv[1] = tmp_outdir;
	ret = subproc_call("mkdir", mkdir_argv, NULL);
	if (ret)
		goto error_wget;

	sprintf(tmp_downdir, "%s/%s", plug_down_dir, download_name);
	tar_argv[1] = tmp_downdir;
	tar_argv[2] = tmp_outdir;
	ret = subproc_call("tar", tar_argv, NULL);

error_wget:
	free(tmp_downdir);
error_alloc_downdir:
	free(tmp_outdir);
	return ret;
}

#endif  /* _INCLUDE_CBENCH_EXEC_HELPER_H_ */
