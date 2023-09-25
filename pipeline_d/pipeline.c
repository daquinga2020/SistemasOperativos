#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <sys/wait.h>

enum {
	MAX_PATH = 64,
	N_PROC = 3,
	READ = 0,		//Entrada estandar, donde leo
	WRITE = 1		//Salida estandar, donde escribo
};

struct Cmd {
	int n_words;
	char **argv_cmd;
	char str_cmd[MAX_PATH];
	char *wd_cop;
};

typedef struct Cmd Cmd;

int
check_num_args(int n_args)
{
	return n_args == 4;
}

int
n_wd(char *set_wd)
{
	char *tok;
	char *wd = malloc(strlen(set_wd) + 1);
	char *p_wd = wd;
	int count;

	if (wd == NULL) {
		err(EXIT_FAILURE, "error");
	}

	strncpy(wd, set_wd, strlen(set_wd) + 1);
	tok = strtok_r(p_wd, " ", &p_wd);

	for (count = 0; tok != NULL; count++) {
		tok = strtok_r(p_wd, " ", &p_wd);
	}
	free(wd);

	return count;
}

void
divide_words(int max_wd, char *copy, char **div_wd)
{
	int i;
	char *token;

	for (i = 0; i < max_wd + 1; i++) {
		token = strtok_r(copy, " ", &copy);
		div_wd[i] = token;
	}
}

int
init_cmd(Cmd * cmd, char *param1)
{
	cmd->n_words = n_wd(param1);
	cmd->wd_cop = malloc(strlen(param1) + 1);
	cmd->argv_cmd = malloc((cmd->n_words + 1) * sizeof(char *));

	if (cmd->argv_cmd == NULL || cmd->wd_cop == NULL) {
		fprintf(stderr, "malloc failed\n");
		return 0;
	}

	strncpy(cmd->wd_cop, param1, strlen(param1) + 1);
	divide_words(cmd->n_words, cmd->wd_cop, cmd->argv_cmd);

	return 1;
}

int
join_path_cmd(char *path, Cmd * cmd)
{
	int sz;
	int n1;

	sz = strlen(cmd->argv_cmd[0]) + strlen(path) + 2;
	n1 = snprintf(cmd->str_cmd, sz, "%s/%s", path, cmd->argv_cmd[0]);
	if (n1 > sz || n1 < 0) {
		fprintf(stderr, "error processing %s%s\n", path,
			cmd->argv_cmd[0]);
		return 0;
	}
	return 1;
}

int
check_files_cmds(int n_params, Cmd * cmds)
{
	int i;
	int j;
	int exist = 0;
	char *files[2] = { "/bin", "/usr/bin" };

	for (i = 0; i < n_params; i++) {
		for (j = 0; j < 2; j++) {
			if (cmds[i].argv_cmd[0][0] != '/') {
				if (!join_path_cmd(files[j], &cmds[i])) {
					return 0;
				}
			}
			if (access(cmds[i].str_cmd, F_OK) != -1) {
				exist = 1;
				break;
			}
		}
		if (!exist) {
			fprintf(stderr,
				"error: file %s doesn't exist\n",
				cmds[i].str_cmd);
			return 0;
		}
	}

	return 1;
}

void
child_proc(int ind, int n_params, int *fd1, int *fd2, Cmd * cmds)
{
	if (!ind) {
		close(fd1[READ]);
	} else {
		if (dup2(fd2[READ], 0) < 0) {
			err(EXIT_FAILURE, "error");
		}
		close(fd2[READ]);
		close(fd1[READ]);
	}
	if (ind != n_params - 1) {
		if (dup2(fd1[WRITE], 1) < 0) {
			err(EXIT_FAILURE, "error");
		}
		close(fd1[WRITE]);
	}

	execv(cmds[ind].str_cmd, cmds[ind].argv_cmd);
}

int
exec_proc(int n_params, pid_t * pids_ch, Cmd * cmds)
{
	int status;
	int i;
	int fds[N_PROC - 1][2];
	pid_t pid_ch;

	for (i = 0; i < n_params; i++) {
		if (i != n_params - 1) {
			if (pipe(fds[i]) < 0) {
				return 1;
			}
		}

		if ((pid_ch = fork()) == 0) {
			child_proc(i, n_params, fds[i], fds[i - 1], cmds);
			return 1;
		} else if (pid_ch > 0) {
			pids_ch[i] = pid_ch;
			if (i) {
				close(fds[i - 1][READ]);
			}
			if (i != n_params - 1) {
				close(fds[i][WRITE]);
			}
		} else {
			perror("fork");
			return 1;
		}
	}

	for (i = 0; i < N_PROC; i++) {
		waitpid(pids_ch[i], &status, 0);
		if (WIFEXITED(status) && i == N_PROC - 1) {
			return WEXITSTATUS(status);
		}
	}

	return 1;
}

void
free_memory_cmd(Cmd * cmds, int n_cmd)
{
	int i;

	for (i = 0; i < n_cmd; i++) {
		free(cmds[i].argv_cmd);
		free(cmds[i].wd_cop);
	}
	free(cmds);
}

int
main(int argc, char *argv[])
{
	int i;
	int type_exit;
	pid_t pids[N_PROC];
	Cmd *cmds = malloc(sizeof(Cmd) * (argc - 1));

	if (cmds == NULL) {
		errx(EXIT_FAILURE, "malloc failed");
	}
	if (!check_num_args(argc)) {
		errx(EXIT_FAILURE, "usage: pipeline cmd1 cmd2 cmd3");
	}

	for (i = 1; i < argc; i++) {
		if (!init_cmd(&cmds[i - 1], argv[i])) {
			errx(EXIT_FAILURE, "error: Cmd can not be initialized");
		}
	}

	//Check files /bin/ and /usr/bin/
	if (!check_files_cmds(argc - 1, cmds)) {
		free_memory_cmd(cmds, argc - 1);
		exit(EXIT_FAILURE);
	}
	type_exit = exec_proc(argc - 1, pids, cmds);

	//Free memory
	free_memory_cmd(cmds, argc - 1);

	exit(type_exit);
}
