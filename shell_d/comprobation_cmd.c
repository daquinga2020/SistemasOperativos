#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include "structs.h"
#include "proc_txt.h"
#include "processing.h"

int
check_fich_reg(unsigned char type)
{
	return type == DT_REG;
}

int
check_btn_cd(int n_params, Cmd * cmds)
{
	int i;

	for (i = 0; i < n_params; i++) {
		if (strcmp(cmds[i].str_cmd, "cd") == 0) {
			return i;
		}
	}
	return -1;
}

int
check_execfile(Cmd * cmd)
{
	DIR *dir;
	struct dirent *p_dir;
	char f_exec[MAX_CMD_F];
	int n1;

	if ((dir = opendir(".")) == NULL) {
		perror("error");
		return 0;
	}

	while ((p_dir = readdir(dir)) != NULL) {
		if (check_fich_reg(p_dir->d_type)) {
			n1 = snprintf(f_exec, MAX_CMD_F, "./%s", p_dir->d_name);
			if (n1 > MAX_CMD_F || n1 < 0) {
				fprintf(stderr, "error processing %s\n",
					p_dir->d_name);
				closedir(dir);
				return 0;
			}
			if (strcmp(cmd->str_cmd, f_exec) == 0) {
				closedir(dir);
				return 1;
			}
		}
	}
	closedir(dir);

	return 0;
}

int
check_cmds(int n_params, Cmd * cmds, VarPath * vp)
{
	int i;
	int j;
	int exist = 0;

	for (i = 0; i < n_params; i++) {
		for (j = 0; j < vp->n_dir_entpath; j++) {
			if (check_execfile(&cmds[i])) {
				exist = 1;
				break;
			}
			if (cmds[i].argv_cmd[0][0] != '/') {
				if (!join_path_cmd
				    (vp->div_entpath[j], &cmds[i])) {
					return 0;
				}
			}
			if (access(cmds[i].str_cmd, F_OK) == 0) {
				exist = 1;
				break;
			}
		}

		if (!exist) {
			fprintf(stderr,
				"error: cmd %s doesn't exist\n",
				cmds[i].argv_cmd[0]);
			return 0;
		}
		exist = 0;
	}

	return 1;
}

int
check_btin_if(char *line)
{
	int f_btn = 0;
	char copy[MAX_LINE];
	char *token;
	char *sv;

	strncpy(copy, line, strlen(line) + 1);
	token = strtok_r(copy, " ", &sv);

	if (strcmp(token, "ifok") == 0) {
		f_btn = 1;
	} else if (strcmp(token, "ifnot") == 0) {
		f_btn = 2;
	}

	while (token != NULL) {
		token = strtok_r(NULL, " ", &sv);
		if (token != NULL
		    && (strcmp(token, "ifok") == 0
			|| strcmp(token, "ifnot") == 0)) {
			fprintf(stderr, "usage: %s cmd [arg]...\n", token);
			return -1;
		}
	}

	return f_btn;
}
