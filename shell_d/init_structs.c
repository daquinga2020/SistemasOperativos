#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include "structs.h"
#include "proc_txt.h"
#include "processing.h"
#include "init_structs.h"

void
init_redirs(Redirs * rs, char *line_cmd)
{
	char copy[MAX_LINE];

	strncpy(copy, line_cmd, strlen(line_cmd) + 1);

	rs->descr_f_entr = 0;
	rs->descr_f_exit = 0;
	sv_redir(copy, '>', rs->f_exit, &rs->red_exit);
	sv_redir(copy, '<', rs->f_entr, &rs->red_entr);
}

void
init_varpath(VarPath * vp)
{
	char *var_entpath = getenv("PATH");

	vp->cpy_vpath[0] = '\0';

	strncpy(vp->cpy_vpath, var_entpath, strlen(var_entpath) + 1);
	vp->n_dir_entpath = n_wd(vp->cpy_vpath, ":");

	vp->div_entpath = malloc(sizeof(char *) * (vp->n_dir_entpath + 1));
	if (vp->div_entpath == NULL) {
		err(EXIT_FAILURE, "error");
	}

	divide_words(vp->cpy_vpath, ":", vp->div_entpath);
}

void
init_pipes(Pipes * p, char *line_cmd)
{
	int i;

	p->fds = NULL;
	p->n_pipes = identify_elem(line_cmd, '|');
	p->set_cmds = malloc(sizeof(char *) * (p->n_pipes + 1));
	if (p->set_cmds == NULL) {
		err(EXIT_FAILURE, "error");
	}

	p->set_cmds[0] = line_cmd;
	if (p->n_pipes) {
		p->fds = malloc(sizeof(int *) * p->n_pipes);
		if (p->fds == NULL) {
			err(EXIT_FAILURE, "error");
		}

		for (i = 0; i < p->n_pipes; i++) {
			p->fds[i] = malloc(sizeof(int) * 2);
			if (p->fds[i] == NULL) {
				err(EXIT_FAILURE, "error");
			}
			p->set_cmds[i] = NULL;
		}

		divide_words(line_cmd, "|", p->set_cmds);
	}
}

void
init_cmd(Cmd * cmd, char *line_cmd, char *sep, Redirs * rs, int last)
{
	cmd->flag_hdoc = 0;
	cmd->flag_amp = find_amps(line_cmd);
	cmd->n_words = n_wd(line_cmd, sep);

	if (rs->red_entr && last) {
		cmd->n_words--;
	}
	if (rs->red_exit && last) {
		cmd->n_words--;
	}

	cmd->argv_cmd = malloc(sizeof(char *) * (cmd->n_words + 1));
	if (cmd->argv_cmd == NULL) {
		err(EXIT_FAILURE, "error");
	}

	divide_words(line_cmd, sep, cmd->argv_cmd);
	cmd->argv_cmd[cmd->n_words] = NULL;

	if (!rs->red_entr && !rs->red_exit && !cmd->flag_amp) {
		if (strcmp(cmd->argv_cmd[cmd->n_words - 1], "HERE{") == 0) {
			cmd->argv_cmd[cmd->n_words - 1] = NULL;
			cmd->flag_hdoc = 1;
		}
	}
	strncpy(cmd->str_cmd, cmd->argv_cmd[0], strlen(cmd->argv_cmd[0]) + 1);
}

int
init_hdoc(HDoc * hd)
{
	char line[MAX_LINE];

	hd->doc[0] = '\0';

	while (fgets(line, MAX_LINE, stdin) != NULL) {
		if (strcmp(line, "}\n") == 0) {
			break;
		}
		strncat(hd->doc, line, MAX_HDOC);
	}
	if (ferror(stdin)) {
		fprintf(stderr, "fgets failed");
		return 0;
	}

	if (pipe(hd->fd_hd) < 0) {
		perror("error");
		return 0;
	}

	return 1;
}
