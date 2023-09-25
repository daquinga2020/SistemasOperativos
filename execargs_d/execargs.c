#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

enum {
	MAX_PAL_ERROR = 100
};

void
fuera_rang_int(int n)		// Si n se encuentra fuera de rango, n será -1 y se saldra con error.
{
	char *s_err = "error: Numero de segundos introducidos fuera de rango\n";

	if (n == -1) {
		fprintf(stderr, "%s", s_err);
		exit(EXIT_FAILURE);
	}
}

int
conv_num(char *num_str)		//      Convierte el string num_str a entero y lo devuelve, si num_str contiene letras se devuelve -2.
						//      Si al convertir el entero se encuentra el entero se encuentra fuera de rango o es negativo, devuelve -1
{
	char *last_word = '\0';

	int num;

	num = strtol(num_str, &last_word, 10);

	if (num < 0) {
		num = -1;
	} else if (*last_word != '\0') {
		num = -2;
	}

	return num;
}

int
rev_uso(int n_params, int seg)	//      Devuelve true(1) si se han cumplido los requisitos de uso. En caso contrario, devuelve false(0)
{
	int valido = 0;

	if ((n_params - 1) < 2 || seg == -2) {
		valido = 1;
	}

	return valido;
}

void
cond_uso(int uso)		// Si no se cumple los requisitos de uso el programa no se ejecuta e imprime su forma correcta de uso por pantalla.
{
	if (uso) {
		fprintf(stderr, "usage: execargs secs command [command ...]\n");
		exit(EXIT_FAILURE);
	}
}

int
n_palb(char *conj_pal)		//      Devuelve un entero con el número de palabras separadas por " " que forma el string concatenado
{
	char *tok;
	char *palbr = malloc(strlen(conj_pal) + 1);
	char *p_palb = palbr;

	int cont;

	strcpy(palbr, conj_pal);
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
main(int argc, char *argv[])
{
	int salida_hijo;
	int salida = EXIT_SUCCESS;
	int i;
	int seg = conv_num(argv[1]);
	int max_pal = 1;
	int valido = rev_uso(argc, seg);

	pid_t pid_hijo;

	char **argv2 = NULL;
	char *palb_cop = NULL;
	char *msg_error = malloc(sizeof(char) * MAX_PAL_ERROR);

	msg_error[0] = '\0';

	strcat(msg_error, "error: ");

	cond_uso(valido);
	fuera_rang_int(seg);

	for (i = 2; i < argc; i++) {

		pid_hijo = fork();

		if (pid_hijo == 0) {
			max_pal = n_palb(argv[i]);
			argv2 = malloc((max_pal + 1) * sizeof(char *));
			palb_cop = malloc(strlen(argv[i]) + 1);

			strcpy(palb_cop, argv[i]);
			sep_conj_palb(max_pal, palb_cop, argv2);

			execv(argv2[0], argv2);

			strcat(msg_error, argv2[0]);
			perror(msg_error);
			salida = EXIT_FAILURE;
			break;

		} else if (pid_hijo > 0) {

			pid_hijo = wait(&salida_hijo);

			if (WEXITSTATUS(salida_hijo)) {
				salida = EXIT_FAILURE;
				fprintf(stderr, "error: %s\n", argv[i]);
				break;
			}

			sleep(seg);

		} else {
			perror("error");
			salida = EXIT_FAILURE;
			break;
		}
	}

	free(argv2);
	free(palb_cop);
	free(msg_error);

	exit(salida);
}
