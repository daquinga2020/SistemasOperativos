#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

struct Matriz {
	int filas;
	int columnas;

	char ***tabla;
};

typedef struct Matriz Matriz;

void
rev_fich(int n_params, int indice, char **params)
{
	int i;
	int fichero;

	for (i = indice + 1; i < n_params; i++) {
		fichero = access(params[i], F_OK);

		if (fichero == -1) {
			fprintf(stderr, "error processing file %s\n",
				params[i]);
			exit(EXIT_FAILURE);
		}
	}
}

int
rev_uso(int n_params, char **params)	//      Devuelve la posicion de la flag si se han cumplido los requisitos de uso. En caso contrario, devuelve false(0)
{
	int valido = 0;
	int i;

	char *flag = "-f";

	if ((n_params - 1) >= 3) {

		for (i = 2; i < n_params; i++) {
			if (strcmp(flag, params[i]) == 0) {
				valido = i;
			} else if (params[i][0] == '-' && params[i][2] == '\0') {
				valido = 0;
				break;
			}
		}
	}

	return valido;
}

void
cond_uso(int uso)		// Si no se cumple los requisitos de uso el programa no se ejecuta e imprime su forma correcta de uso por pantalla.
{
	if (!uso) {
		fprintf(stderr,
			"usage: grepmatrix word [words ...] -f file [file ...]\n");
		exit(EXIT_FAILURE);
	}
}

int
cont_fich(int n_params, int ind)	//  Devuelve el número de ficheros
{
	int cont = 0;
	int i;

	for (i = ind; i < n_params - 1; i++) {
		cont++;
	}

	return cont;
}

Matriz
crea_matriz(int n_fich, int n_palb)
{
	Matriz temp;

	temp.filas = n_fich + 1;
	temp.columnas = n_palb + 1;

	int i;

	temp.tabla = malloc(sizeof(char **) * temp.filas);

	for (i = 0; i < temp.filas; i++) {
		temp.tabla[i] = malloc(sizeof(char *) * temp.columnas);
	}

	return temp;
}

int
compr_malloc(Matriz m)
{
	int correcto = 1;
	int i;
	int j;

	for (i = 0; i < m.filas; i++) {
		for (j = 0; j < m.columnas; j++) {
			if (m.tabla[i] == NULL) {
				correcto = 0;
				break;
			}
		}
		if (correcto == 0) {
			break;
		}
	}

	return correcto;
}

void
err_malloc(int func)
{
	if (!func) {
		fprintf(stderr, "error: no hay suficiente espacio memoria\n");
		exit(EXIT_FAILURE);
	}
}

void
rellenar_matriz(int n_params, int pos_flag, char **params, Matriz * m)
{
	int i;
	int j;
	int flag = pos_flag;

	for (i = 0; i < m->filas; i++) {
		for (j = 0; j < m->columnas; j++) {
			if (i == 0 && j == m->columnas - 1) {
				m->tabla[i][j] = "\0";
			} else if (i == 0 && j != flag) {
				m->tabla[i][j] = params[j + 1];
			} else if (i != 0 && j == m->columnas - 1) {
				flag++;
				m->tabla[i][j] = params[flag];
			} else {
				m->tabla[i][j] = "o";
			}
		}
	}
}

void
impr_matriz(Matriz * m, int pos_flag)
{
	int i;
	int j;

	for (i = 0; i < m->filas; i++) {
		for (j = 0; j < m->columnas; j++) {
			if (i == 0 && j != pos_flag && j != m->columnas - 1) {
				printf("\"%s\"\t", m->tabla[i][j]);
			} else {
				printf("%s\t", m->tabla[i][j]);
			}

		}
		printf("\n");
	}
}

void
liberar_matriz(Matriz * m)
{
	int i;

	for (i = 0; i < m->filas; i++) {
		free(m->tabla[i]);
	}

	free(m->tabla);
}

void
crea_proc(int n_params, int flag_lim, pid_t * hijos_proc, char **params)
{
	int cont = 0;
	int i;
	int j;

	char *fich;

	pid_t pid_hijo;

	for (i = flag_lim + 1; i < n_params; i++) {
		for (j = 1; j < flag_lim; j++) {
			pid_hijo = fork();

			if (pid_hijo == 0) {
				fich =
				    malloc(sizeof(char) *
					   (strlen(params[i]) + 1));
				fich = params[i];
				execl("/bin/fgrep", "fgrep", "-q", "-s", "-w",
				      params[j], fich, NULL);
				free(fich);
				exit(EXIT_FAILURE);
			} else if (pid_hijo > 0) {
				hijos_proc[cont] = pid_hijo;
				cont++;
			} else {
				perror("fork");
				exit(EXIT_FAILURE);
			}
		}
	}
}

int
main(int argc, char *argv[])
{
	int salida = EXIT_SUCCESS;
	int flag = rev_uso(argc, argv);
	int n_palb = flag - 1;
	int n_fich = cont_fich(argc, flag);
	int func_mall;
	int salida_hijo;
	int ind_c = 0;
	int ind_f = 1;
	int cont = 0;
	int huecos_xo;

	//int i;
	//int j;

	//pid_t pid_hijo;

	//char *fich;

	Matriz matriz;

	cond_uso(flag);
	rev_fich(argc, flag, argv);

	matriz = crea_matriz(n_fich, n_palb);

	func_mall = compr_malloc(matriz);
	err_malloc(func_mall);

	rellenar_matriz(argc, flag, argv, &matriz);

	huecos_xo = (matriz.filas - 1) * (matriz.columnas - 1);

	pid_t *hijos = malloc(sizeof(pid_t) * huecos_xo);

	/*for (i = flag+1; i < argc; i++) {
	   for(j = 1; j < flag; j++){
	   pid_hijo = fork();

	   if(pid_hijo == 0)
	   {   
	   fich = malloc(sizeof(char)*(strlen(argv[i])+1));
	   fich = argv[i];
	   execl("/bin/fgrep", "fgrep", "-q", "-s", argv[j], fich, NULL);
	   exit(1);
	   }else if(pid_hijo > 0){
	   hijos[cont] = pid_hijo;
	   cont++;
	   }else {
	   perror("fork");
	   salida = EXIT_FAILURE;
	   break;
	   }
	   }
	   } */

	crea_proc(argc, flag, hijos, argv);

	while (cont < huecos_xo) {
		waitpid(hijos[cont], &salida_hijo, 0);

		if (WIFEXITED(salida_hijo)) {
			if (!WEXITSTATUS(salida_hijo)) {
				matriz.tabla[ind_f][ind_c] = "x";
			}

			cont++;
			ind_c++;

			if (ind_c == n_palb) {
				ind_c = 0;
				ind_f++;
			}
		}
	}

	impr_matriz(&matriz, flag);

	liberar_matriz(&matriz);
	free(hijos);

	exit(salida);
}
