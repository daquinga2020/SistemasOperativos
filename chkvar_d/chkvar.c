#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>

void
uso(int n)			// Si no se cumple el minimo de parámetros de entrada el programa no se ejecuta y sale con error
{
	if ((n - 1) < 2 || (n - 1) % 2 != 0) {
		fprintf(stderr,
			"usage: ch varname varcontent [varname varcontent] ...\n");
		exit(EXIT_FAILURE);
	}
}

int
tipo_salida(int n_error)
{
	if (n_error == 0) {
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}

int
comp_var_entorn_contenido(char *nomb_entorn, char *nomb_conten)	// Devuelve true(1) si la nomb_entorn(var de entorno) es igual a nomb_conten, false(0) en caso contrario
{
	char *var_entorno;

	int error = 0;

	var_entorno = getenv(nomb_entorn);

	if (var_entorno == NULL || strcmp(var_entorno, nomb_conten) != 0) {
		error = 1;
	}

	return error;
}

void
impr_error_parejas(int n_elem, int n_errores, char **pareja_val_cont)	// Imprime un error según las parejas de variable de entorno y contenido que no son iguales 
{
	if (n_errores != 0) {
		int err;
		int i;

		fprintf(stderr, "error:");

		for (i = 1; i < n_elem; i = i + 2) {
			err =
			    comp_var_entorn_contenido(pareja_val_cont[i],
						      pareja_val_cont[i + 1]);
			if (err) {
				fprintf(stderr, " %s != %s", pareja_val_cont[i],
					pareja_val_cont[i + 1]);
				if (n_errores > 1) {
					fprintf(stderr, ",");
				}
				n_errores--;
			}
		}
		fprintf(stderr, "\n");
	}
}

int
cont_err_var(int n_elem, char **pareja_val_cont)	// Devuelve n_errores si en pareja_val_cont las variables de entorno(indice impar)
												// coinciden con el contenido(indice par).
{
	int cont = 0;
	int i;

	for (i = 1; i < n_elem; i = i + 2) {

		if (comp_var_entorn_contenido
		    (pareja_val_cont[i], pareja_val_cont[i + 1])) {
			cont++;
		}
	}

	return cont;
}

int
main(int argc, char *argv[])
{
	uso(argc);

	int n_err = cont_err_var(argc, argv);

	impr_error_parejas(argc, n_err, argv);

	int salida = tipo_salida(n_err);

	exit(salida);
}
