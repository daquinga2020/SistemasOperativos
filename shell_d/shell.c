#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include "structs.h"
#include "proc_txt.h"
#include "init_structs.h"
#include "processing.h"
#include "comprobation_cmd.h"

int
main(int argc, char *argv[])
{
	int i;
	int btn_cd;
	int last_cmd;
	int result = EXIT_SUCCESS;
	int n_var;
	char line[MAX_LINE];

	Cmd *cmds;
	Pipes p;
	VarPath vp;
	Redirs rs;
	HDoc hd;

	while (fgets(line, MAX_LINE, stdin) != NULL) {
		if (line[strlen(line) - 1] == '\n') {
			line[strlen(line) - 1] = '\0';	// To delete newline
		}
		last_cmd = 0;

		//Update variable result
		setresult(result);

		//Environment variable substitution
		if ((n_var = identify_elem(line, '$'))) {
			if ((result = replace_var(n_var, line))) {
				continue;
			}
		}
		//Execution builtin ifok/ifnot
		if ((result = exec_ifbtn(line))) {
			continue;
		}

		init_varpath(&vp);
		init_redirs(&rs, line);
		init_pipes(&p, line);

		//Check sintaxis for pipes
		if (!check_setpipes(&p)) {
			fprintf(stderr, "bad sintaxis\n");
			free_pipes(&p);
			free(vp.div_entpath);
			continue;
		}

		cmds = malloc(sizeof(Cmd) * (p.n_pipes + 1));
		if (cmds == NULL) {
			err(EXIT_FAILURE, "error");
		}
		//Set environment variables
		if ((result = set_var(line, &cmds[0], &rs))) {
			if (result == 1) {
				result = 0;
			}
			free_cmd(cmds, p.n_pipes + 1);
			free_pipes(&p);
			free(vp.div_entpath);
			continue;
		}

		for (i = 0; i < (p.n_pipes + 1); i++) {
			if (i == p.n_pipes) {
				last_cmd = 1;
			}
			init_cmd(&cmds[i], p.set_cmds[i], " <>\t&", &rs,
				 last_cmd);
		}

		//Check sintaxis for ampersand
		if (cmds[p.n_pipes].flag_amp < 0) {
			fprintf(stderr, "error: bad sintaxis\n");
			free_cmd(cmds, p.n_pipes + 1);
			free_pipes(&p);
			free(vp.div_entpath);
			continue;
		}
		//Execution builtin 
		if ((btn_cd = check_btn_cd(p.n_pipes + 1, cmds)) >= 0) {
			result = exec_cd(&cmds[btn_cd]);
			free_cmd(cmds, p.n_pipes + 1);
			free_pipes(&p);
			free(vp.div_entpath);
			continue;
		}
		//Check if cmd or cmds exist
		if (!check_cmds(p.n_pipes + 1, cmds, &vp)) {
			result = 1;
			free_cmd(cmds, p.n_pipes + 1);
			free_pipes(&p);
			free(vp.div_entpath);
			continue;
		}
		//Initialize struct HDoc if there is here document
		if (cmds[p.n_pipes].flag_hdoc) {
			if (!init_hdoc(&hd)) {
				free_cmd(cmds, p.n_pipes + 1);
				free_pipes(&p);
				free(vp.div_entpath);
				continue;
			}
		}
		//Open redirections
		open_redirs(&rs, cmds[p.n_pipes].flag_amp);

		//Execution of N cmds
		result = exec_cmds(&p, cmds, &rs, &hd);

		//Free memory
		free_cmd(cmds, p.n_pipes + 1);
		free_pipes(&p);
		free(vp.div_entpath);
	}
	if (!feof(stdin)) {
		errx(EXIT_FAILURE, "eof not reached");
	}

	exit(result);
}
