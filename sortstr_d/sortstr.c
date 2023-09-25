#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
no_param(int n)
{
	if (n == 1) {
		fprintf(stderr,
			"No se han introducido parámetros de entrada\n");
		exit(EXIT_FAILURE);
	}
}

int
detectar_vocal(char *palabra)
{
	char vocales[] = "aeiouAEIOU";
	int detectada = 0;

	// Recorrer las vocales y comparar si la letra es una de ellas

	for (int i = 0; i < strlen(vocales); i++) {
		for (int j = 0; j < strlen(palabra); j++) {
			if (palabra[j] == vocales[i]) {
				detectada = 1;
				return detectada;
			}
		}
	}

	// Si terminamos de recorrer y no regresamos true dentro del for, entonces no es vocal
	return detectada;
}

void
insertion_sort(char **palabras, int n)
{
	int i = 2;
	int j = 1;

	while (i < n) {
		j = i - 1;

		char *aux = palabras[i];

		while (j >= 1 && strcmp(palabras[j], aux) > 0) {
			palabras[j + 1] = palabras[j];
			j = j - 1;
		}
		palabras[j + 1] = aux;
		i = i + 1;
	}
}

int
main(int argc, char *argv[])
{
	no_param(argc);

	insertion_sort(argv, argc);
	for (int i = 1; i < argc; i++) {
		if (detectar_vocal(argv[i]) != 0) {
			printf("%s\n", argv[i]);
		}
	}

	exit(EXIT_SUCCESS);
}
