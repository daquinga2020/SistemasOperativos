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

	char **tabla;
};

typedef struct Matriz Matriz;

void
rev_fich(int n_params, int indice, char **params)	//      Comprueba si existe un fichero, si no existe el programa termina con error.
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

int
comprb_malloc(Matriz m)
{
	return m.tabla == NULL;
}

void
err_malloc(int func)
{
	if (func) {
		fprintf(stderr, "error: no hay suficiente espacio memoria\n");
		exit(EXIT_FAILURE);
	}
}

Matriz
crea_matriz(int n_fich, int n_palb)
{
	Matriz temp;

	temp.filas = n_fich + 1;
	temp.columnas = n_palb + 1;

	temp.tabla = malloc(sizeof(char *) * (temp.filas * temp.columnas - 1));

	return temp;
}

void
rellenar_matriz(int n_params, int pos_flag, char **params, Matriz * m)	//      Rellena la matriz con "x", "o", las palabras y los ficheros 
{
	int i;
	int tot = (m->filas * m->columnas) - 1;
	int ind_params = 1;
	int ind_x_o = 0;
	int cont_x_o = m->columnas - 1;

	for (i = 0; i < tot; i++) {
		if (i < pos_flag - 1) {
			m->tabla[i] = params[ind_params];
			ind_params++;
		} else if (ind_x_o < cont_x_o) {
			m->tabla[i] = "o";
			ind_x_o++;
		} else {
			ind_params++;
			m->tabla[i] = params[ind_params];
			ind_x_o = 0;
		}
	}
}

void
impr_matriz(Matriz * m, int pos_flag)	//      Imprime la matriz
{
	int i;
	int tot = (m->filas * m->columnas) - 1;
	int sep = m->columnas - 2;

	for (i = 0; i < tot; i++) {
		if (i < pos_flag - 1) {
			printf("\"%s\"\t", m->tabla[i]);
		} else {
			printf("%s\t", m->tabla[i]);

		}
		if (i == sep) {
			printf("\n");
			sep = m->columnas + sep;
		}
	}
}

void
crea_ejec_proc(int n_params, int flag_lim, pid_t * pid_hijos, char **params)	// Crea un proceso hijo que realiza la búsqueda de una palabra con fgrep
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
				pid_hijos[cont] = pid_hijo;
				cont++;
			} else {
				perror("fork");
				exit(EXIT_FAILURE);
			}
		}
	}
}

void
esp_proc(pid_t * pid_hijos, int salida_hijo, int n_proc, Matriz * m)	//      Espera a que los "n_proc" procesos terminen
{
	int cont = 0;
	int encontrada = m->columnas - 1;
	int palbs = m->columnas - 1;
	int salto = 1;

	while (cont < n_proc) {
		waitpid(pid_hijos[cont], &salida_hijo, 0);

		if (WIFEXITED(salida_hijo)) {
			if (!WEXITSTATUS(salida_hijo)) {
				m->tabla[encontrada] = "x";
			}

			cont++;
			encontrada++;

			if (cont == palbs * salto) {
				encontrada++;
				salto++;
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
	int salida_hijo = 0;
	int huecos_xo;

	Matriz matriz;

	pid_t *hijos;

	cond_uso(flag);
	rev_fich(argc, flag, argv);

	matriz = crea_matriz(n_fich, n_palb);

	func_mall = comprb_malloc(matriz);
	err_malloc(func_mall);

	rellenar_matriz(argc, flag, argv, &matriz);

	huecos_xo = (matriz.filas - 1) * (matriz.columnas - 1);
	hijos = malloc(sizeof(pid_t) * huecos_xo);

	crea_ejec_proc(argc, flag, hijos, argv);
	esp_proc(hijos, salida_hijo, huecos_xo, &matriz);

	impr_matriz(&matriz, flag);

	free(matriz.tabla);
	free(hijos);

	exit(salida);
}
