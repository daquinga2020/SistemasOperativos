#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


enum {
	MAX_LINE = 256,
	MAX_CMD_F = 128,
	MAX_PATH = 512,
	READ = 0,		//Entrada estandar, donde leo
	WRITE = 1		//Salida estandar, donde escribo
};

struct Cmd {
	int n_words;
	int red_entr;
	char **argv_cmd;
	char str_cmd[MAX_CMD_F];
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
check_n_args(int n_args)
{
	return n_args == 2;
}

int
n_palb(char *phrase, char *sep)
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

void	//Intentar que no necesite copia
divide_words(int max_wd, char *phrase, char *sep, char **div_wd)
{
	int i;
	char *token;

	for (i = 0; i < max_wd + 1; i++) {
		token = strtok_r(phrase, sep, &phrase);
		div_wd[i] = token;
	}
	/*
	   int i;
	   char *token;
	   char *cop_phrase = malloc(strlen(phrase) + 1);
	   char *p_cop_phrase = cop_phrase;

	   strncpy(cop_phrase, phrase, strlen(phrase) + 1);

	   for (i = 0; i < max_wd + 1; i++) {
	   token = strtok_r(p_cop_phrase, sep, &p_cop_phrase);
	   //fprintf(stderr, "%s %ld\n", token, strlen(token));
	   if(token != NULL) {
	   div_wd[i] = malloc(strlen(token) + 1);
	   strncpy(div_wd[i], token, strlen(token) + 1);
	   }
	   //div_wd[i] = token;

	   }
	   fprintf(stderr, "Sali\n");
	   free(cop_phrase); //-> Problema al liberarlo
	 */
}

void
prueba(int max_wds, char *phrase, char *sep, char **div_wd)
{
	int i;

	char *str1;
	char *token;
	char *saveptr1;
	char copy[MAX_LINE];
	
	strncpy(copy, phrase, strlen(phrase)+1);

	for (i = 0, str1 = copy; i < max_wds; i++, str1 = NULL) {
		div_wd[i] = NULL;
        token = strtok_r(str1, sep, &saveptr1);
		
		div_wd[i] = token;
		fprintf(stderr, "Token: %s\n", div_wd[i]);
	}

	div_wd[max_wds] = NULL;
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

int
init_varpath(VarPath *vp)
{
	char *var_entpath = getenv("PATH");
	vp->n_dir_entpath = n_palb(var_entpath, ":");
	vp->div_entpath = malloc(sizeof(char *) * (vp->n_dir_entpath + 1));
	if (vp->div_entpath == NULL) {
		fprintf(stderr, "malloc failed\n");
		return 0;
	}
	
	divide_words(vp->n_dir_entpath, var_entpath, ":", vp->div_entpath);
	//prueba(vp->n_dir_entpath, var_entpath, ":", vp->div_entpath);
	
	return 1;
}

int
init_pipes(Pipes *p, char *line_cmd)
{
	int i;

	p->n_pipes = identify_pipe(line_cmd);
	p->pids = NULL;
	p->fds = NULL;

	if (p->n_pipes) {
		p->pids = malloc(sizeof(pid_t)*(p->n_pipes+1));
		p->fds = malloc(sizeof(int*)*p->n_pipes);

		if (p->pids == NULL || p->fds == NULL) {
			fprintf(stderr, "malloc failed\n");
			return 0;
		}

		for(i = 0; i < p->n_pipes; i++) {
			p->fds[i] = malloc(sizeof(int)*2);
			if (p->fds[i] == NULL) {
				fprintf(stderr, "malloc failed\n");
				return 0;
			}
		}
	}

	return 1;
}

int
init_cmd(Cmd * cmd, char *param1)
{
	cmd->n_words = n_palb(param1, " ");
	cmd->argv_cmd = malloc((cmd->n_words + 1) * sizeof(char *));

	if (cmd->argv_cmd == NULL) {
		fprintf(stderr, "malloc failed\n");
		return 0;
	}

	divide_words(cmd->n_words, param1, " ", cmd->argv_cmd);
	//prueba(cmd->n_words, param1, " ", cmd->argv_cmd);
	//fprintf(stderr, "Tmn de 1 arg %s: %ld\n", cmd->argv_cmd[0], strlen(cmd->argv_cmd[0]));
	strncpy(cmd->str_cmd, cmd->argv_cmd[0], strlen(cmd->argv_cmd[0]) + 1);

	return 1;
}

int
join_path_cmd(char *path, Cmd * cmd)	// Cambiarlo en el pipeline
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

	for (i = 0; i < n_params; i++) {
		if (access(cmds[i].str_cmd, F_OK) == -1) {
			fprintf(stderr,
				"error: file %s doesn't exist\n",
				cmds[i].str_cmd);
			return 0;
		}
	}
	return 1;
}

int
ejec_n_cmd(int n_cmds, int **fds, pid_t * pids_ch, Cmd * cmds)
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
				close(fds[i][READ]);
			} else {
				dup2(fds[i - 1][READ], 0);
				close(fds[i - 1][READ]);
			}
			if (i != n_cmds - 1) {
				dup2(fds[i][WRITE], 1);
				close(fds[i][WRITE]);
			}

			execv(cmds[i].str_cmd, cmds[i].argv_cmd);
			return 1;
		} else if (pid_ch > 0) {
			pids_ch[i] = pid_ch;
			if (i) {
				close(fds[i - 1][READ]);
			}
			if (i != n_cmds - 1) {
				close(fds[i][WRITE]);
			}
		} else {
			perror("fork");
			return 1;
		}
	}

	for (i = 0; i < n_cmds; i++) {	//Meter en una funcion
		waitpid(pids_ch[i], &status, 0);
		if (WIFEXITED(status) && i == n_cmds - 1) {
			return WEXITSTATUS(status);
		}
	}

	return 1;
}

int
ejec_cmd(Cmd *cmd)
{
	int status;

	switch (fork())
	{
	case -1:
		perror("fork");
		return 1;
		break;
	case 0:
		execv(cmd->str_cmd, cmd->argv_cmd);
		fprintf(stderr, "execv failed\n");
		return 1;
		break;
	default:
		wait(&status);
		break;
	}

	return WEXITSTATUS(status);
}

int
detect_redd(char **args_cmd, char *red, int n_wd)
{
	int i;
	int ind = -1;

	for(i = 0; i < n_wd; i++) {
		if(strcmp(args_cmd[i], red) == 0) {
			ind = i;
			fprintf(stderr, "Hay una redireccion en la posición: %d\n", i);
		}
	}
	return ind;
}

int
main(int argc, char *argv[])
{
	// Bucle de fgets
	int n = 0;
	int words;
	int exist = 0;
	int i;
	int j;
	/////////////////
	

	//Coidgo para testar funcion prueba
	char str1[] = "HOLA MANOLO COMO ANDAMOS POR AQUI";
	int wd = 6;
	char **lista = malloc(sizeof(char*)*(wd+1));
	prueba(wd, str1, " ", lista);

	fprintf(stderr, "Palabra intacta: %s\n", str1);
	
	for(i = 0; i < wd; i++) {
		fprintf(stderr, "%s\n", lista[i]);
	}

	free(lista);
	/////////////////////////////////////////////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//Estructura Pipes
	Pipes p;
	/////////////////////////////////////////////////////////////////////////////////////////////////////

	int type_exit;
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//Estructura Cmd
	char **ch_aux_cmds = NULL;
	char aux_line[MAX_LINE];
	char line[MAX_LINE];
	
	Cmd *cmds;
	/////////////////////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// Variable de entorno PATH
	VarPath vp;

	//	Inicializacion de la variable path
	init_varpath(&vp);
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////


	while (fgets(line, MAX_LINE, stdin) != NULL) {

		if (line[strlen(line) - 1] == '\n') {
			line[strlen(line) - 1] = '\0';	// To delete newline
		}
		init_pipes(&p, line);
		cmds = malloc(sizeof(Cmd) * (p.n_pipes + 1));


		words = n_palb(line, " ");	// Unicamente para informar acerca de los comandos se puede quitar
		//	Informacion del comando que se va a tratar
		fprintf(stderr,
			"Lectura %d de la entrada estandar: %s\n\t# %d palabras y %d pipes\n\n",
			n, line, words, p.n_pipes);


		if (p.n_pipes) {	//	Incializacion de n estructuras Cmd, Se encuentran en el array cmds

			strncpy(aux_line, line, strlen(line) + 1);

			//	Para almacenar los strings que se encuentran separados por "|"
			ch_aux_cmds = malloc(sizeof(char *) * (p.n_pipes + 1));

			divide_words(p.n_pipes, aux_line, "|", ch_aux_cmds);	//Revisar funcion fuga de memoria
			//prueba(p.n_pipes+1, line, "|", ch_aux_cmds);

			/////////////////////////////////////////////////////////////////////////////
			//Inicializacion de Estructuras Cmd
			for (i = 0; i < (p.n_pipes + 1); i++) {

				if (!init_cmd(&cmds[i], ch_aux_cmds[i])) {
					exit(EXIT_FAILURE);
				}
			}
			/////////////////////////////////////////////////////////////////////////////

		} else {

			/////////////////////////////////////////////////////////////////////////////
			//Inicializacion de una unica Estructura Cmd para un solo comando
			if (!init_cmd(&cmds[0], line)) {
				exit(EXIT_FAILURE);
			}
			/////////////////////////////////////////////////////////////////////////////
			
		}

		


		/////////////////////////////////////////////////////////////////////////////////
		//Revisa existencia de comandos en los directorios de la variable PATH

		for(i = 0; i < p.n_pipes+1; i++) {
			for (j = 0; j < vp.n_dir_entpath; j++) {
				
				//Si no es una ruta absoluta se une el comando con un directorio de PATH
				if(cmds[i].argv_cmd[0][0] != '/') {
					join_path_cmd(vp.div_entpath[j], &cmds[i]);
				}

				//	Se comprueba si el fichero existe, si existe sale del segundo for
				if (access(cmds[i].str_cmd, F_OK) != -1) {
					exist = 1;
					break;
				}

			}


			//	Si no existe el fichero ejecutable del comando en los directorios de la variable PATH
			if (!exist) {
				fprintf(stderr,
					"error: file %s doesn't exist\n",
					cmds[i].str_cmd);
				exit(EXIT_FAILURE);	//Liberar cosas antes de salir
			}

		}
		/////////////////////////////////////////////////////////////////////////////////

		

		/////////////////////////////////////////////////////////////////////////////////
		if (p.n_pipes) {

			//Ejecucion para mas de un comando
			type_exit = ejec_n_cmd(p.n_pipes+1, p.fds, p.pids, cmds);

		} else {

			//Ejecucion para un comando

			cmds[0].red_entr = detect_redd(cmds[0].argv_cmd, "<", cmds[0].n_words);
			type_exit = ejec_cmd(&cmds[0]);
		}
		/////////////////////////////////////////////////////////////////////////////////

		
		
		n++;
		fprintf(stderr, "\n\n");

		/////////////////////////////////////////////////////////////////////////////////
		//Free memory
		for (i = 0; i < p.n_pipes + 1; i++) {
			free(cmds[i].argv_cmd);
		}
		free(cmds);	//liberacion de la memoria del array de Estructura Cmd

		if (p.n_pipes) {
			for(i = 0; i < p.n_pipes; i++) {
				free(p.fds[i]);
			}
			free(p.fds);
			free(p.pids);	// Liberacion de la memoria reservada por Estructura Pipes
			free(ch_aux_cmds);
		}
		/////////////////////////////////////////////////////////////////////////////////
	}

	if (!feof(stdin)) {
		fprintf(stderr, "eof not reached");
		return 0;
	}
	free(vp.div_entpath);	// Variable de entorno PATH

	exit(type_exit);
}

//Cambiar nombres de funcion n_palb y sep_conj_palb en pipeline.c


/*De la funcion divide_words

	
	   int i;

	   char *token;
	   char *cop_phrase = malloc(strlen(phrase) + 1);

	   char *p_cop_phrase = cop_phrase;

	   strncpy(cop_phrase, phrase, strlen(phrase) + 1);

	   for (i = 0; i < max_wd + 1; i++) {
	   token = strtok_r(p_cop_phrase, sep, &p_cop_phrase);

	   if(token != NULL) {
	   div_wd[i] = malloc(strlen(token) + 1);
	   if (div_wd[i] == NULL) {
	   fprintf(stderr, "malloc failed\n");
	   }
	   strncpy(div_wd[i], token, strlen(token) + 1);
	   } else {
	   div_wd[i] = NULL;
	   }
	   }

	   free(cop_phrase);
	 
*/