#ifndef _INCLUDE_CBENCH_EXEC_HELPER_H_
#define _INCLUDE_CBENCH_EXEC_HELPER_H_

#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include <cbench/plugin.h>

static inline int subproc_call_get_stdout(const char *bin, char * const argv[],
		char **out)
{
	int status;
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
		dup2(pipefd[1], 1);
		execvp(bin, argv);
		_exit(-1);
	} else {
		size_t out_len = 512;
		size_t off = 0;

		*out = malloc(out_len);

		if (!*out)
			goto exec_error;

		close(pipefd[1]);

		while (1) {
			size_t read_len = read(pipefd[0], *out + off, out_len - off);

			if (read_len == 0)
				goto end_of_stream;
			if (read_len < 0)
				goto exec_error;

			off += read_len;
			if (off >= out_len - 1) {
				out_len = 2 * out_len;
				*out = realloc(*out, out_len);
				if (!*out)
					goto exec_error;
			}
		}

end_of_stream:
		close(pipefd[0]);
		if (pid != waitpid(pid, &status, 0))
			return -1;
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


static int subproc_call(const char *bin, char * const argv[])
{
	int status;
	pid_t pid;

	pid = fork();

	if (pid == -1)
		return -1;
	if (pid == 0) {
		int null = open("/dev/null", O_WRONLY);
		dup2(null, 1);
		execvp(bin, argv);
		printf("Failed to execute %s: %s\n", bin, strerror(errno));
		_exit(-1);
	} else {
		if (pid != waitpid(pid, &status, 0))
			return -1;
	}
	return status;
}

static inline int cbench_wget(struct plugin *plug, char *address, char *out_name)
{
	char *argv[] = {"wget", "-O", out_name, address, "-q", NULL};
	const char *out_dir = plugin_get_download_dir(plug);
	char *out_buf;

	out_buf = malloc(strlen(out_name) + strlen(out_dir) + 2);
	sprintf(out_buf, "%s/%s", out_dir, out_name);

	if (!access(out_buf, F_OK | R_OK)) {
		free(out_buf);
		return 0;
	}

	argv[2] = out_buf;

	return subproc_call("wget", argv);
}

static inline int cbench_wget_untar(struct plugin *plug,
		char *address, char *download_name,
		char *out_dir, char *tar_dir)
{
	char *tar_argv[] = {"tar", "-xf", download_name, "-C", out_dir, tar_dir, NULL};
	char *mkdir_argv[] = {"mkdir", "-p", out_dir, NULL};
	const char *plug_work_dir = plugin_get_work_dir(plug);
	const char *plug_down_dir = plugin_get_download_dir(plug);
	char *tmp_outdir;
	char *tmp_downdir;
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
	mkdir_argv[2] = tmp_outdir;
	ret = subproc_call("mkdir", mkdir_argv);
	if (ret)
		goto error_wget;

	sprintf(tmp_downdir, "%s/%s", plug_down_dir, download_name);
	tar_argv[2] = tmp_downdir;
	tar_argv[4] = tmp_outdir;
	ret = subproc_call("tar", tar_argv);

error_wget:
	free(tmp_downdir);
error_alloc_downdir:
	free(tmp_outdir);
	return ret;
}

/* Remove path recursively. Path is interpreted relative to working dir. */
static inline int cbench_rm_rec(struct plugin *plug, const char *path)
{
	char *rm_args[] = {"rm", "-Rf", NULL, NULL};
	const char *work_dir = plugin_get_work_dir(plug);
	char *abs_path = malloc(strlen(path) + strlen(work_dir) + 2);
	int ret;

	if (!abs_path)
		return -1;

	sprintf(abs_path, "%s/%s", work_dir, path);
	rm_args[2] = abs_path;

	ret = subproc_call("rm", rm_args);
	free(abs_path);
	return ret;
}

/* Remove path recursively. Path is interpreted relative to working dir. */
static inline int cbench_mkdir(struct plugin *plug, const char *path)
{
	char *mkdir_args[] = {"mkdir", "-p", NULL, NULL};
	const char *work_dir = plugin_get_work_dir(plug);
	char *abs_path = malloc(strlen(path) + strlen(work_dir) + 2);
	int ret;

	if (!abs_path)
		return -1;

	sprintf(abs_path, "%s/%s", work_dir, path);
	mkdir_args[2] = abs_path;

	ret = subproc_call("mkdir", mkdir_args);
	free(abs_path);
	return ret;
}

#endif  /* _INCLUDE_CBENCH_EXEC_HELPER_H_ */
