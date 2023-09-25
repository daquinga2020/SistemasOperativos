#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include "structs.h"
#include "proc_txt.h"
#include "init_structs.h"
#include "processing.h"
#include "comprobation_cmd.h"

int
open_redirs(Redirs * rs, int flag_amps)
{
	if (rs->red_exit) {
		rs->descr_f_exit =
		    open(rs->f_exit, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	}
	if (rs->red_entr) {
		rs->descr_f_entr = open(rs->f_entr, O_RDONLY);
	} else if (flag_amps) {
		rs->descr_f_entr = open("/dev/null", O_RDONLY);
	}

	if (rs->descr_f_exit < 0 || rs->descr_f_entr < 0) {
		perror("error");
		return 0;
	}

	return 1;
}

void
apply_redirs(int flag_redir, int fd1, int fd2)
{
	if (flag_redir) {
		if (dup2(fd1, fd2) < 0) {
			err(EXIT_FAILURE, "error");
		}
		close(fd1);
	}
}

void
proc_son(int index, Pipes * p, Redirs * rs)
{
	if (!index) {
		close(p->fds[index][READ]);
	} else {
		if (dup2(p->fds[index - 1][READ], 0) < 0) {
			err(EXIT_FAILURE, "error");
		}
		close(p->fds[index - 1][READ]);
	}

	if (index != p->n_pipes) {
		if (dup2(p->fds[index][WRITE], 1) < 0) {
			err(EXIT_FAILURE, "error");
		}
		close(p->fds[index][WRITE]);
		close(p->fds[index][READ]);
	}
}

void
proc_fth(int index, Pipes * p, Redirs * rs, int fd_hd, int flag_amps,
	 int flag_hd)
{
	if (index) {
		close(p->fds[index - 1][READ]);
	} else if (rs->red_entr || flag_amps) {
		close(rs->descr_f_entr);
		rs->red_entr = 0;
		flag_amps = 0;
	} else if (flag_hd && !index) {
		close(fd_hd);
	}

	if (index != p->n_pipes) {
		close(p->fds[index][WRITE]);
	} else if (rs->red_exit) {
		close(rs->descr_f_exit);
		rs->red_exit = 0;
	}
}

int
exec_cmds(Pipes * p, Cmd * cmds, Redirs * rs, HDoc * hd)
{
	int status = 0;
	int w_pid = 0;
	int rest = 0;
	int i;

	for (i = 0; i < p->n_pipes + 1; i++) {
		if (i != p->n_pipes) {
			if (pipe(p->fds[i]) < 0) {
				perror("error");
				return 1;
			}
		}
		if ((p->last_pid = fork()) == 0) {
			if (!i) {
				apply_redirs(rs->red_entr, rs->descr_f_entr, 0);
				apply_redirs(cmds[p->n_pipes].flag_hdoc,
					     hd->fd_hd[READ], 0);
				if (cmds[p->n_pipes].flag_hdoc) {
					close(hd->fd_hd[WRITE]);
				}
			}
			if (p->n_pipes) {
				proc_son(i, p, rs);
			}
			if (i == p->n_pipes) {
				apply_redirs(rs->red_exit, rs->descr_f_exit, 1);
			}
			execv(cmds[i].str_cmd, cmds[i].argv_cmd);
			perror("error");
			return EXIT_FAILURE;
		} else if (p->last_pid > 0) {
			if (!i && cmds[p->n_pipes].flag_hdoc && !wr_heredoc(hd)) {
				return 1;
			}
			proc_fth(i, p, rs, hd->fd_hd[READ],
				 cmds[p->n_pipes].flag_amp,
				 cmds[p->n_pipes].flag_hdoc);
		} else {
			err(EXIT_FAILURE, "error");
		}
	}

	if (cmds[p->n_pipes].flag_amp) {
		return status;
	}

	do {
		w_pid = waitpid(p->last_pid, &status, 0);
	} while (w_pid != -1 && !WIFEXITED(status));

	if (w_pid == -1) {
		fprintf(stderr, "wait failed\n");
		return EXIT_FAILURE;
	}

	while (wait(&rest) > 0) {
		;
	}

	return WEXITSTATUS(status);
}

int
exec_cd(Cmd * cmd)
{
	int error;

	if(cmd->n_words != 2) {
		fprintf(stderr, "cd: too many arguments\n");
		return EXIT_FAILURE;
	}

	if (cmd->argv_cmd[1] != NULL) {
		error = chdir(cmd->argv_cmd[1]);
	} else {
		error = chdir(getenv("HOME"));
	}

	if (error < 0) {
		perror("error");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void
free_cmd(Cmd * cmds, int n)
{
	int i;

	for (i = 0; i < n; i++) {
		free(cmds[i].argv_cmd);
	}
	free(cmds);
}

void
free_pipes(Pipes * p)
{
	int i;

	if (p->n_pipes) {
		for (i = 0; i < p->n_pipes; i++) {
			free(p->fds[i]);
		}
		free(p->fds);
	}
	free(p->set_cmds);
}

int
set_var(char *line, Cmd * cmd, Redirs * rs)
{
	int stat = 1;

	if (strchr(line, '=') != NULL) {
		if (!check_cmd_setvar(line)) {
			fprintf(stderr, "usage: var=STRING [STRING]...\n");
			stat = -1;
		}
		init_cmd(cmd, line, "=", rs, 0);

		if (setenv(cmd->argv_cmd[0], cmd->argv_cmd[1], 0) < 0) {
			perror("error");
			stat = -1;
		}
	} else {
		stat = 0;
	}

	return stat;
}

int
wr_heredoc(HDoc * hd)
{
	int sz = strlen(hd->doc) + 1;

	if ((write(hd->fd_hd[WRITE], hd->doc, sz)) != sz) {
		fprintf(stderr, "error: cannot write\n");
		return 0;
	}
	close(hd->fd_hd[WRITE]);

	return 1;
}

int
setresult(int result)
{
	int error;

	error = setenv("result", "0", 1);
	if (result < 0 || result) {
		error = setenv("result", "1", 1);
	}

	if (error < 0) {
		perror("error");
		return 0;
	}

	return 1;
}

int
exec_ifbtn(char *line)
{
	int builtin;
	char *sv;
	char *res;
	char copy[MAX_LINE];

	copy[0] = '\0';
	builtin = check_btin_if(line);

	if (builtin < 0) {
		return -1;
	} else if (!builtin) {
		return 0;
	}

	strncpy(copy, line, strlen(line) + 1);
	strtok_r(copy, " ", &sv);
	strncpy(line, sv, strlen(sv) + 1);

	res = getenv("result");
	if (res == NULL) {
		fprintf(stderr, "error: var result does not exist\n");
		return -1;
	}

	if (builtin == 1) {	//ifok
		if (strcmp(res, "0") == 0) {
			return 0;
		}
	} else if (builtin == 2) {	//ifnot
		if (strcmp(res, "1") == 0) {
			return 0;
		}
	}

	return 1;
}
