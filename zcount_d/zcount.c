#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <err.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

enum {
	BUFFER = 8 * 1024
};

int
cmpr_fich_reg(unsigned char type)
{
	return type == DT_REG;
}

int
cont_byt_zero(char *byt, int nbyt_leid)
{
	int i;
	int cont = 0;

	for (i = 0; i < nbyt_leid; i++) {
		if (byt[i] == 0) {
			cont++;
		}
	}
	return cont;
}

int
main(int argc, char *argv[])
{
	int descr_z;
	int descr_fich_d;
	int nr;
	int cont = 0;

	DIR *dir;
	struct dirent *p_dir;

	char *file_z = "z.txt";
	char bff[BUFFER];

	if (!(argc == 2)) {
		fprintf(stderr, "usage: zcount dir\n");
		exit(EXIT_FAILURE);
	}

	dir = opendir(argv[1]);
	if (dir == NULL) {
		if (errno == ENOENT) {
			errx(EXIT_FAILURE, "usage: zcount dir");
		}
		errx(EXIT_FAILURE, "opendir failed: %s", argv[1]);
	}

	if (chdir(argv[1]) < 0) {
		errx(EXIT_FAILURE, "chdir failed");
	}

	descr_z = open(file_z, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (descr_z < 0) {
		err(EXIT_FAILURE, "%s", file_z);
	}

	if (dup2(descr_z, 1) < 0) {
		err(EXIT_FAILURE, "error");
	}
	close(descr_z);

	p_dir = readdir(dir);
	while (p_dir != NULL) {
		if (!cmpr_fich_reg(p_dir->d_type)
		    || strcmp(p_dir->d_name, file_z) == 0) {
			p_dir = readdir(dir);
			continue;
		}

		descr_fich_d = open(p_dir->d_name, O_RDONLY);
		if (descr_fich_d < 0) {
			err(EXIT_FAILURE, "%s", p_dir->d_name);
		}

		while ((nr = read(descr_fich_d, &bff, BUFFER)) != 0) {
			if (nr < 0) {
				err(EXIT_FAILURE, "error");
			}
			cont = cont + cont_byt_zero(bff, nr);
		}
		printf("%d\t%s\n", cont, p_dir->d_name);
		close(descr_fich_d);
		cont = 0;
		p_dir = readdir(dir);
	}
	closedir(dir);
	exit(EXIT_SUCCESS);
}
