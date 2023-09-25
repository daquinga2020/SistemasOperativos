
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

enum {
	BUFFER = 8 * 1024,
	SIZE_RUTA = 20
};

struct Copybytes {
	int exs_byt;		//Flag para saber si hay un tercer párametro que indique el numero de bytes
	int bytes;
	int entr_est;		//Flag para saber si hay que leer de la entrada estandar
	int sald_est;		//Flag para saber si hay que escribir en la salida estandar
	int descr_fich_org;
	int descr_fich_dest;
	int byts_leidos;
	int byts_escritos;

	char fich_org[SIZE_RUTA];
	char fich_dest[SIZE_RUTA];
	char bff[BUFFER];
};

typedef struct Copybytes Copybytes;

void
fuera_rang_int(int n)		// Si n se encuentra fuera de rango o no es valido se saldra con error.
{
	char *s_err =
	    "error: Numero de bytes introducidos no válido o fuera de rango\n";

	if (n == -1) {
		fprintf(stderr, "%s", s_err);
		exit(EXIT_FAILURE);
	}
}

int
exist_n_bytes(int n_params)
{
	int existe = 0;

	if (n_params - 1 == 3)
		existe = 1;

	return existe;
}

int
conv_num(char *num_str)		//      Convierte el string num_str a entero y lo devuelve, si contiene letras, se encuentra fuera de rango
						    //       o es negativo se devuelve -1.
{
	char *last_word = '\0';

	int num;

	num = strtol(num_str, &last_word, 10);

	if (num < 0 || *last_word != '\0')
		num = -1;

	return num;
}

int
rev_uso(int n_params, char **params)	//  Devuelve 1 si se cumplen las condicones de uso. En caso contrario devuelve 0.
{
	int valido = 0;

	if ((n_params - 1) == 2 || (n_params - 1) == 3)
		valido = 1;
	else
		valido = 0;

	return valido;
}

void
cond_uso(int uso)		// Si no se cumple los requisitos de uso el programa no se ejecuta e imprime su forma correcta de uso por pantalla.
{
	if (!uso) {
		fprintf(stderr,
			"usage: copybytes fich_org|- fich_dest|- [bytes]\n");
		exit(EXIT_FAILURE);
	}
}

void
compr_descr_fich(Copybytes * cpy_by)
{
	char *fich_err = NULL;

	if (!cpy_by->entr_est && cpy_by->descr_fich_org == -1)
		fich_err = cpy_by->fich_org;
	else if (!cpy_by->sald_est && cpy_by->descr_fich_dest == -1)
		fich_err = cpy_by->fich_dest;

	if (fich_err != NULL) {
		perror("Error al abrir el fichero:");
		exit(EXIT_FAILURE);
	}
}

Copybytes
crea_copybytes(int n_params, char **params)
{
	Copybytes tmp;
	struct stat stat_f;

	tmp.exs_byt = exist_n_bytes(n_params);

	strcpy(tmp.fich_org, params[1]);
	strcpy(tmp.fich_dest, params[2]);

	if (strcmp(tmp.fich_org, "-") == 0) {
		tmp.entr_est = 1;
		tmp.descr_fich_org = 0;
	} else {
		tmp.descr_fich_org = open(tmp.fich_org, O_RDONLY);
		tmp.entr_est = 0;
	}

	if (strcmp(tmp.fich_dest, "-") == 0) {
		tmp.sald_est = 1;
		tmp.descr_fich_dest = 1;
	} else {
		tmp.descr_fich_dest =
		    open(tmp.fich_dest, O_CREAT | O_TRUNC | O_WRONLY, 0644);
		tmp.sald_est = 0;
	}

	compr_descr_fich(&tmp);
	stat(tmp.fich_org, &stat_f);

	if (tmp.exs_byt) {
		tmp.bytes = conv_num(params[3]);
		fuera_rang_int(tmp.bytes);

		if (tmp.bytes > stat_f.st_size && !tmp.entr_est)
			tmp.bytes = stat_f.st_size;

	} else {
		tmp.bytes = stat_f.st_size;
		if (tmp.entr_est)
			tmp.bytes = BUFFER;
	}

	return tmp;
}

void
lect_escr_fichs(Copybytes * cpy_by)
{
	cpy_by->byts_leidos =
	    read(cpy_by->descr_fich_org, cpy_by->bff, cpy_by->bytes);

	while (cpy_by->byts_leidos != 0) {
		if (cpy_by->byts_leidos < 0) {
			fprintf(stderr, "No se ha podido leer\n");
			exit(EXIT_FAILURE);
		}

		cpy_by->byts_escritos =
		    write(cpy_by->descr_fich_dest, cpy_by->bff,
			  cpy_by->byts_leidos);

		if (cpy_by->byts_escritos != cpy_by->byts_leidos) {
			fprintf(stderr, "No se ha podido escribir\n");
			exit(EXIT_FAILURE);
		}
		cpy_by->byts_leidos =
		    read(cpy_by->descr_fich_org, cpy_by->bff, cpy_by->bytes);
	}
}

void
cierre_fichs(Copybytes * cpy_byt)
{
	int cerrado1;
	int cerrado2;

	cerrado1 = close(cpy_byt->descr_fich_org);
	cerrado2 = close(cpy_byt->descr_fich_dest);

	if (cerrado1 == -1 || cerrado2 == -1) {
		fprintf(stderr,
			"No se ha podido cerrar un fichero correctamente.\n");
		exit(EXIT_FAILURE);
	}
}

int
main(int argc, char *argv[])
{
	Copybytes copybytes;

	int valido = rev_uso(argc, argv);

	cond_uso(valido);

	copybytes = crea_copybytes(argc, argv);
	lect_escr_fichs(&copybytes);
	cierre_fichs(&copybytes);

	exit(EXIT_SUCCESS);
}
