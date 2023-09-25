
	//int fd1[2];
	//int fd2[2];
	//int fd3[2];
/*
	pipe(fd1);                 // comunica ls y grep /
	
    pid = fork();     

    if(pid == 0)              //hijo 1 INICIAL
    {      		
        close(fd1[READ]);   // cerrar extremo no necesario
		
        dup2(fd1[WRITE], STDOUT_FILENO);
		close(fd1[WRITE]);
		
        execlp("/bin/ls", "ls", "-l", NULL);
    }
    else                          // padre 
    { 
		close(fd1[WRITE]);    // extremo no necesario ya 
		
		pipe(fd2);		  //comunica grep y grep 
        pid = fork();
        
       if(pid == 0)               // hijo 2 INTERMEDIO
       	{
	  		close(fd2[READ]);   // cerrar extremo no necesario 
									
          	dup2(fd1[READ], STDIN_FILENO);
	  		close(fd1[READ]);
			
	  		dup2(fd2[WRITE], STDOUT_FILENO);
	  		close(fd2[WRITE]);
			
          	execlp("/bin/grep","grep", "u",NULL);
		}
		else // padre
		{
        	close(fd1[READ]);      //cerrar extremo no necesario
			close(fd2[WRITE]);     //cerrar extremo no necesario

			pipe(fd3);		  // comunica grep y wc 
     		pid = fork();

	   		if(pid == 0) // hijo 3/	//INTERMEDIO
        	{
				close(fd3[READ]);   // cerrar extremo no necesario 

				dup2(fd2[READ], STDIN_FILENO);
				close(fd2[READ]);

				dup2(fd3[WRITE], STDOUT_FILENO);
	  			close(fd3[WRITE]);

				execlp("/bin/grep","grep", "u",NULL);
	  		}
			else
			{
				close(fd2[READ]);     // cerrar extremo no necesario 
				close(fd3[WRITE]);    // extremo no necesario ya 
				
				pid = fork();

				if(pid == 0) ///hijo 4 FINAL
				{
					dup2(fd3[READ], STDIN_FILENO);
					close(fd3[READ]);
					execlp("/usr/bin/wc","wc", "-l",NULL);
				}
			}
        }
    }

    close(fd3[0]);  //cerrar extremo que queda abierto en el padre
	
   //wait para cada hijo
    wait(&status);
    wait(&status);
    wait(&status);
	wait(&status);
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <sys/wait.h>

enum {
	MAX_PATH=30,
	N_PROC=3,
	READ=0,	//Entrada estandar, donde leo
	WRITE=1	//Salida estandar, donde escribo
};

struct Cmd {
	int n_words;
	char **argv_cmd;
	char *str_cmd;
    char *palb_cop;
};

typedef struct Cmd Cmd;

int
check_num_args(int n_args)
{
	return n_args == 4;
}

int
n_palb(char *conj_pal)		//      Devuelve un entero con el número de palabras separadas por " " que forma el string concatenado
{
	char *tok;
	char *palbr = malloc(strlen(conj_pal) + 1);
	char *p_palb = palbr;
	int cont;

	strncpy(palbr, conj_pal, strlen(conj_pal) + 1);
	tok = strtok_r(p_palb, " ", &p_palb);

	for (cont = 0; tok != NULL; cont++) {
		tok = strtok_r(p_palb, " ", &p_palb);
	}
	free(palbr);

	return cont;
}

void
sep_conj_palb(int max_pal, char *copia, char **palb_sep)	//      Separa las palabras por " " que se encuentran en copia y las
															//      almacena en palb_sep incluyendo un NULL en su últipa posicion.
{
	int i;
	char *token;

	for (i = 0; i < max_pal + 1; i++) {
		token = strtok_r(copia, " ", &copia);
		palb_sep[i] = token;
	}
}

int
init_cmd(Cmd * cmd, char *param1)
{
	cmd->n_words = n_palb(param1);
    cmd->palb_cop = malloc(strlen(param1) + 1);
	cmd->argv_cmd = malloc((cmd->n_words + 1) * sizeof(char *));
	cmd->str_cmd = malloc(MAX_PATH);

	if (cmd->argv_cmd == NULL || cmd->palb_cop == NULL) {
		fprintf(stderr, "malloc failed\n");
        return 0;
	}

	strncpy(cmd->palb_cop, param1, strlen(param1) + 1);
	sep_conj_palb(cmd->n_words, cmd->palb_cop, cmd->argv_cmd);
	
	return 1;
}

int
check_path_cmd(char *path, Cmd *cmd)
{
	int sz;
	int n1;

	sz = strlen(cmd->argv_cmd[0]) + strlen(path) + 1;
	n1 = snprintf(cmd->str_cmd, sz,"%s%s", path, cmd->argv_cmd[0]);
	if (n1 > sz || n1 < 0) {
		fprintf(stderr, "error processing %s%s\n", path, cmd->argv_cmd[0]);
		return 0;
	}
	return access(cmd->str_cmd, F_OK) != 1;
}

int
check_files_cmds(int n_params, Cmd *cmds)
{
	int i;
	char *bin = "/bin/";
	char *b_usr = "/usr/bin/";

	for (i = 0; i < n_params; i++) {
		if(cmds[i].argv_cmd[0][0] != '/') {
			if (!check_path_cmd(bin, &cmds[i])) {
				if(!check_path_cmd(b_usr, &cmds[i])) {
					fprintf(stderr, "error: file %s doesn't exist\n", cmds[i].str_cmd);
					return 0;
				}
			}
			//strncpy(cmds[i].argv_cmd[0], cmds[i].str_cmd, strlen(cmds[i].str_cmd)+1); //A lo mejor no hace falta revisar despues de ejecutar el exec
		} else {
			strncpy(cmds[i].str_cmd, cmds[i].argv_cmd[0], MAX_PATH);
			if(access(cmds[i].str_cmd, F_OK) == -1) {
				fprintf(stderr, "error: file %s doesn't exist\n", cmds[i].str_cmd);
				return 0;
			}
		}
	}
	return 1;
}

int
ejec_proc(int n_params, pid_t *pids_ch, Cmd *cmds)
{
	int status;
	int i;
	int fds[N_PROC-1][2];
	pid_t pid_ch;

	//Utilizar el for y crear una funcion child_middle
	for (i = 0; i < n_params; i++) {

		if (i != n_params-1) {
			//fprintf(stderr, "Pipe %d creado\n", i);
			if (pipe(fds[i]) < 0) {
				return 0;
			}
		}
		
		if ((pid_ch = fork()) == 0) {

			if (!i) {
				close(fds[i][READ]);	//	Solo lo ejecuta el primer hijo
			} else {
				dup2(fds[i-1][READ], 0);
				close(fds[i-1][READ]);
			}

			if (i != n_params-1) {
				dup2(fds[i][WRITE], 1);
				close(fds[i][WRITE]);
			}
			//fprintf(stderr, "Soy el hijo con pid: %d\n", getpid());
			execv(cmds[i].str_cmd, cmds[i].argv_cmd);
			
			return 0;

		} else if (pid_ch > 0) {
			pids_ch[i] = pid_ch;
			//fprintf(stderr, "Hijo %d creado con pid: %d\n", i, pids_ch[i]);

			if (i) {
				close(fds[i-1][READ]);
			}
			if(i != n_params-1) {
				close(fds[i][WRITE]);
			}

		} else {
			perror("fork");
			return 0;
		}
	}
	//fprintf(stderr, "\n\n");
	for (i = 0; i < N_PROC; i++) {
		//fprintf(stderr, "Esperare al hijo %d con pid: %d\n", i, pids_ch[i]);
		waitpid(pids_ch[i], &status, 0);
		//fprintf(stderr, "Hijo %d con pid: %d su WIFEXITED(status) es: %d\n", i, pids_ch[i], WIFEXITED(status));
		if (WIFEXITED(status) && i == N_PROC-1) {
			//fprintf(stderr, "Estado del hijo %d: %d\n", i, WEXITSTATUS(status));
			return WEXITSTATUS(status);
		}
	}

	return 1;
}

int
main(int argc, char *argv[])
{
	int i;
    pid_t pids[N_PROC];
	Cmd *cmds = malloc(sizeof(Cmd) * (argc-1));

    if (cmds == NULL) {
        errx(EXIT_FAILURE, "malloc failed");
    }
	if (!check_num_args(argc)) {
		errx(EXIT_FAILURE, "usage: pipeline cmd1 cmd2 cmd3");
	}

    //Meterlo en una funcion
	for (i = 1; i < argc; i++) {
		if(!init_cmd(&cmds[i - 1], argv[i])) {
            errx(EXIT_FAILURE, "usage: pipeline cmd1 cmd2 cmd3");
        }
	}

    //Check files /bin/ and /usr/bin/
    if(!check_files_cmds(argc-1, cmds)) {
		exit(EXIT_FAILURE);
	}
	
	ejec_proc(argc-1, pids, cmds);

	//Free memory
    for(i = 0; i < argc-1; i++){
        free(cmds[i].argv_cmd);
        free(cmds[i].palb_cop);
		free(cmds[i].str_cmd);
    }
    free(cmds);

	exit(EXIT_SUCCESS);
}
