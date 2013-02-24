#ifndef _INCLUDE_CBENCH_EXEC_HELPER_H_
#define _INCLUDE_CBENCH_EXEC_HELPER_H_

#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cbench/plugin.h>

static inline int subproc_call_get_stdout(const char *bin, const char **argv,
					const char **env, char **out)
{
	int status
	pid_t pid;
	int pipefd[2];
	int ret;

	*out = NULL;

	ret = pipe(pipefd);
	if (ret)
		return -1;

	pid = fork();

	if (pid == -1)
		return -1;
	if (pid == 0) {
		close(pipefd[0]);
		freopen(pipefd[1], "w", stdout);
		execve(bin, argv, env);
		_exit(-1);
	} else {
		size_t out_len = 512;
		char **out;
		size_t off = 0;

		*out = malloc(out_len);

		if (!*out)
			goto exec_error;

		close(pipefd[1]);

		while (1) {
			size_t read = read(pipefd[0], *out + off, out_len - off);

			if (read == 0)
				goto end_of_stream;
			if (read < 0)
				goto exec_error;

			off += read;
			if (off >= out_len - 1) {
				out_len = 2 * out_len;
				*out = realloc(*out, out_len);
				if (!*out)
					goto exec_error;
			}
		}

end_of_stream:
		close(pipefd[0]);
		ret = waitpid(pid, &status, 0);
		if (ret) {
			return -1;
		}
	}
	return status;
exec_error:
	close(pipefd[0]);
	waitpid(pid, &status, 0);
	if (*out)
		free(*out);
	*out = NULL;
	return -1;
}


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
