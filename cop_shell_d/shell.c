#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>

enum {
	MAX_LINE = 256,
	MAX_CMD_F = 128,
	MAX_PATH = 512,
	READ = 0,		//Entrada estandar, donde leo
	WRITE = 1		//Salida estandar, donde escribo
};

struct Cmd {
	int flag_amp;
	int red_entr;
	int red_exit;
	int descr_f_entr;
	int descr_f_exit;
	int n_words;
	char **argv_cmd;
	char str_cmd[MAX_CMD_F];
	char *wd_cop;
};

struct Pipes {
	int **fds;
	int n_pipes;
	pid_t *pids;
};

struct VarPath {
	int n_dir_entpath;
	char *var_entpath;
	char **div_entpath;
};

typedef struct Cmd Cmd;
typedef struct Pipes Pipes;
typedef struct VarPath VarPath;

int
n_wd(char *phrase, char *sep)
{
	char *tok;
	char *cop_phrase = malloc(strlen(phrase) + 1);
	char *p_cop_phrase = cop_phrase;
	int count;

	strncpy(cop_phrase, phrase, strlen(phrase) + 1);
	tok = strtok_r(p_cop_phrase, sep, &p_cop_phrase);

	for (count = 0; tok != NULL; count++) {
		tok = strtok_r(p_cop_phrase, sep, &p_cop_phrase);
	}
	free(cop_phrase);

	return count;
}

void
divide_words(int max_wd, char *phrase, char *sep, char **div_wd)
{
	int i;
	char *token;

	for (i = 0; i < max_wd + 1; i++) {
		token = strtok_r(phrase, sep, &phrase);
		div_wd[i] = token;
	}
}

int
identify_pipe(char *line_cmds)
{
	int i;
	int n_pipes = 0;

	for (i = 0; i < strlen(line_cmds); i++) {
		if (line_cmds[i] == '|') {
			n_pipes++;
		}
	}

	return n_pipes;
}

void
replace_var(Cmd * cmd)
{
	int i;
	char *var;
	char *value;

	for (i = 0; i < cmd->n_words; i++) {
		if (cmd->argv_cmd[i][0] == '$') {
			var =
			    strtok_r(cmd->argv_cmd[i], "$", &cmd->argv_cmd[i]);
			if ((value = getenv(var)) != NULL) {
				cmd->argv_cmd[i] = value;
			} else {
				errx(EXIT_FAILURE, "var %s does not exist", var);	//Revisar si salir aqui
			}
		}
	}
}

int
init_varpath(VarPath * vp)
{
	char *var_entpath = getenv("PATH");

	vp->n_dir_entpath = n_wd(var_entpath, ":");
	vp->div_entpath = malloc(sizeof(char *) * (vp->n_dir_entpath + 1));
	if (vp->div_entpath == NULL) {
		fprintf(stderr, "malloc failed\n");
		return 0;
	}

	divide_words(vp->n_dir_entpath, var_entpath, ":", vp->div_entpath);

	return 1;
}

int
init_pipes(Pipes * p, char *line_cmd)
{
	int i;

	p->n_pipes = identify_pipe(line_cmd);
	p->pids = NULL;
	p->fds = NULL;

	if (p->n_pipes) {
		p->pids = malloc(sizeof(pid_t) * (p->n_pipes + 1));
		p->fds = malloc(sizeof(int *) * p->n_pipes);

		if (p->pids == NULL || p->fds == NULL) {
			fprintf(stderr, "malloc failed\n");
			return 0;
		}

		for (i = 0; i < p->n_pipes; i++) {
			p->fds[i] = malloc(sizeof(int) * 2);
			if (p->fds[i] == NULL) {
				fprintf(stderr, "malloc failed\n");
				return 0;
			}
		}
	}

	return 1;
}

int
init_cmd(Cmd * cmd, char *param1, char *sep)
{
	cmd->descr_f_entr = 0;
	cmd->descr_f_exit = 0;
	cmd->red_entr = -1;
	cmd->red_exit = -1;
	cmd->flag_amp = -1;

	cmd->n_words = n_wd(param1, sep);
	cmd->wd_cop = malloc(strlen(param1) + 1);
	cmd->argv_cmd = malloc((cmd->n_words + 1) * sizeof(char *));

	if (cmd->argv_cmd == NULL || cmd->wd_cop == NULL) {
		fprintf(stderr, "malloc failed\n");
		return 0;
	}

	divide_words(cmd->n_words, param1, " ", cmd->argv_cmd);
	replace_var(cmd);
	strncpy(cmd->str_cmd, cmd->argv_cmd[0], strlen(cmd->argv_cmd[0]) + 1);

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
check_pthfiles_cmds(int n_params, Cmd * cmds, VarPath * vp)
{
	int i;
	int j;
	int exist = 0;

	for (i = 0; i < n_params; i++) {
		for (j = 0; j < vp->n_dir_entpath; j++) {
			if (cmds[i].argv_cmd[0][0] != '/') {
				if (!join_path_cmd
				    (vp->div_entpath[j], &cmds[i])) {
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

int
detect_wd(char **args_cmd, char *wd, int n_wd)
{
	int i;
	int ind = -1;

	for (i = 0; i < n_wd; i++) {
		if (strcmp(args_cmd[i], wd) == 0) {
			ind = i;
		}
	}
	return ind;
}

int
check_cmd_setvar(char *wd)
{
	int i;

	for (i = 0; i < strlen(wd); i++) {
		if (wd[i] == '=' && wd[i - 1] != ' ' && wd[i + 1] != ' ') {
			return 1;
		}
	}

	return 0;
}

int
open_redds(Cmd * cmd)
{
	if (cmd->red_exit != -1) {
		cmd->descr_f_exit =
		    open(cmd->argv_cmd[cmd->red_exit + 1],
			 O_WRONLY | O_CREAT | O_TRUNC, 0666);
		cmd->argv_cmd[cmd->red_exit] = NULL;
		cmd->argv_cmd[cmd->red_exit + 1] = NULL;
	}
	if (cmd->red_entr != -1) {
		cmd->descr_f_entr =
		    open(cmd->argv_cmd[cmd->red_entr + 1], O_RDONLY);
		cmd->argv_cmd[cmd->red_entr] = NULL;
		cmd->argv_cmd[cmd->red_entr + 1] = NULL;
	} else if (cmd->flag_amp != -1) {
		cmd->descr_f_entr = open("/dev/null", O_RDONLY);
	}

	if (cmd->descr_f_exit < 0 || cmd->descr_f_entr < 0) {
		return 0;
	}

	return 1;
}

void
apply_redd(int flag_redd, int fd1, int fd2)
{
	if (flag_redd != -1) {
		fprintf(stderr, "Solo debo entrar una vez\n");
		if (dup2(fd1, fd2) < 0) {
			err(EXIT_FAILURE, "error");	//Revisar errores para ver cómo salir
		}
		close(fd1);
	}
}

int
exec_n_cmd(int n_cmds, int **fds, pid_t * pids_ch, Cmd * cmds)
{
	int status;
	int i;

	pid_t pid_ch;

	for (i = 0; i < n_cmds; i++) {
		if (i != n_cmds - 1) {
			if (pipe(fds[i]) < 0) {
				return 1;
			}
		}
		if ((pid_ch = fork()) == 0) {
			if (!i) {
				//apply_redd(cmds[n_cmds - 1].red_entr,
				//	   cmds[n_cmds - 1].descr_f_entr, 0);
				close(fds[i][READ]);
			} else {
				if (dup2(fds[i - 1][READ], 0) < 0) {
					err(EXIT_FAILURE, "error");
				}
				close(fds[i - 1][READ]);
			}
			if (i != n_cmds - 1) {
				if (dup2(fds[i][WRITE], 1) < 0) {
					err(EXIT_FAILURE, "error");
				}
				close(fds[i][WRITE]);
				close(fds[i][READ]);
			} else {
				//apply_redd(cmds[i].red_exit,
				//	   cmds[i].descr_f_exit, 1);
			}
			execv(cmds[i].str_cmd, cmds[i].argv_cmd);
			return EXIT_FAILURE;
		} else if (pid_ch > 0) {
			pids_ch[i] = pid_ch;
			if (i) {	//Cerrar en el padre las redirecciones de entrada y salida
				close(fds[i - 1][READ]);
			}
			if (i != n_cmds - 1) {
				close(fds[i][WRITE]);
				cmds[n_cmds - 1].red_entr = -1;
			}
		} else {
			perror("fork");
			return EXIT_FAILURE;
		}
	}

	if (cmds[n_cmds - 1].flag_amp == -1) {
		for (i = 0; i < n_cmds; i++) {	//Meter en una funcion
			waitpid(pids_ch[i], &status, 0);
			if (WIFEXITED(status) && i == n_cmds - 1) {
				return WEXITSTATUS(status);
			}
		}
	}

	return EXIT_FAILURE;
}

int
exec_cmd(Cmd * cmd)
{
	int status;

	switch (fork()) {
	case -1:
		perror("fork");
		return 1;
		break;
	case 0:
		apply_redd(cmd->red_exit, cmd->descr_f_exit, 1);
		apply_redd(cmd->red_entr, cmd->descr_f_entr, 0);

		execv(cmd->str_cmd, cmd->argv_cmd);
		err(EXIT_FAILURE, "error");
		break;
	default:
		wait(&status);
		break;
	}

	return WEXITSTATUS(status);
}

int
main(int argc, char *argv[])
{
	int type_exit;
	int i;
	char **ch_aux_cmds = NULL;
	char aux_line[MAX_LINE];
	char line[MAX_LINE];
	Cmd *cmds;
	Pipes p;
	VarPath vp;

	if (!init_varpath(&vp)) {
		exit(EXIT_FAILURE);
	}
	while (fgets(line, MAX_LINE, stdin) != NULL) {
		//getcwd(path_current, MAX_PATH);
		if (line[strlen(line) - 1] == '\n') {
			line[strlen(line) - 1] = '\0';	// To delete newline
		}

		if (!init_pipes(&p, line)) {
			exit(EXIT_FAILURE);
		}
		cmds = malloc(sizeof(Cmd) * (p.n_pipes + 1));
		if (cmds == NULL) {
			err(EXIT_FAILURE, "error");
		}

		if (p.n_pipes) {
			strncpy(aux_line, line, strlen(line) + 1);

			ch_aux_cmds = malloc(sizeof(char *) * (p.n_pipes + 1));
			if (ch_aux_cmds == NULL) {
				err(EXIT_FAILURE, "error");
			}
			divide_words(p.n_pipes, aux_line, "|", ch_aux_cmds);
		} else {
			ch_aux_cmds = malloc(sizeof(char *));
			ch_aux_cmds[0] = line;
		}

		for (i = 0; i < (p.n_pipes + 1); i++) {
			if (!init_cmd(&cmds[i], ch_aux_cmds[i], " ")) {
				exit(EXIT_FAILURE);
			}
		}
		//Variables de entorno
		if (cmds->n_words == 1 && strchr(cmds[0].str_cmd, '=') != NULL) {
			if (!check_cmd_setvar(cmds[0].str_cmd)) {
				errx(EXIT_FAILURE, "%s not valid",
				     cmds[0].str_cmd);
			}
			divide_words(cmds[0].n_words, cmds[0].str_cmd,
				     "=", cmds[0].argv_cmd);
			if (setenv
			    (cmds[0].argv_cmd[0], cmds[0].argv_cmd[1], 0) < 0) {
				err(EXIT_FAILURE, "error");
			}
			//Free memory
			for (i = 0; i < p.n_pipes + 1; i++) {
				free(cmds[i].argv_cmd);
				free(cmds[i].wd_cop);
			}
			free(cmds);
			free(ch_aux_cmds);
			continue;
		}
		////////
		
		if (!check_pthfiles_cmds(p.n_pipes + 1, cmds, &vp)) {
			exit(EXIT_FAILURE);
		}

		cmds[p.n_pipes].flag_amp =
		    detect_wd(cmds[p.n_pipes].argv_cmd, "&",
			      cmds[p.n_pipes].n_words);
		if (cmds[p.n_pipes].flag_amp != -1) {
			cmds[p.n_pipes].argv_cmd[cmds[p.n_pipes].flag_amp] =
			    NULL;
			cmds[p.n_pipes].n_words--;
			//	Falta redireccionar la entrada estandar  /dev/null
		}

		cmds[p.n_pipes].red_exit =
		    detect_wd(cmds[p.n_pipes].argv_cmd, ">",
			      cmds[p.n_pipes].n_words);
		cmds[p.n_pipes].red_entr =
		    detect_wd(cmds[p.n_pipes].argv_cmd, "<",
			      cmds[p.n_pipes].n_words);
		open_redds(&cmds[p.n_pipes]);

		if (p.n_pipes) {
			//Ejecucion para mas de un comando
			type_exit = exec_n_cmd(p.n_pipes + 1, p.fds, p.pids, cmds);	//     Revisar para pasar estructura Pipe
		} else {
			//Ejecucion para un comando
			type_exit = exec_cmd(&cmds[0]);
		}

		//Free memory
		for (i = 0; i < p.n_pipes + 1; i++) {
			free(cmds[i].argv_cmd);
			free(cmds[i].wd_cop);
		}
		free(cmds);	//liberacion de la memoria del array de Estructura Cmd
		free(ch_aux_cmds);
		if (p.n_pipes) {
			for (i = 0; i < p.n_pipes; i++) {
				free(p.fds[i]);
			}
			free(p.fds);
			free(p.pids);	// Liberacion de la memoria reservada por Estructura Pipes      
		}
	}

	if (!feof(stdin)) {
		fprintf(stderr, "eof not reached");
		type_exit = EXIT_FAILURE;
	}
	free(vp.div_entpath);	// Variable de entorno PATH

	exit(type_exit);
}
//No salir nunca aunque falle un comando