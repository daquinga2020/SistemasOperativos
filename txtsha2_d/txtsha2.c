#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

enum {
	READ = 0,
	WRITE = 1,
	BUFFER = 1024 * 8,
	MAX_PATH = 512
};

int
check_n_args(int n_args)
{
	return n_args <= 2;
}

int
check_dir(DIR * dir)
{
	int sv_err_dir;

	if (dir == NULL) {
		sv_err_dir = errno;
		if (sv_err_dir == ENOENT) {
			fprintf(stderr, "usage: txtsha2 [dir]\n");
		} else {
			perror("error");
		}
		return 0;
	}
	return 1;
}

int
check_fich_reg(unsigned char type)
{
	return type == DT_REG;
}

int
check_ext(char *file, char *typ_ext)
{
	char *last_ch = strrchr(file, '.');

	if (last_ch == NULL) {
		return 0;
	}

	return (strcmp(last_ch, typ_ext) == 0);
}

int
join_path_f(char *savepth, char *path, char *file)
{
	int sz;
	int n1;

	sz = strlen(file) + strlen(path) + 2;
	n1 = snprintf(savepth, sz, "%s/%s", path, file);
	if (n1 > sz || n1 < 0) {
		fprintf(stderr, "error processing %s%s\n", path, file);
		return 0;
	}
	return 1;
}

void
exc_sha(int *fd1, int *fd2)
{
	switch (fork()) {
	case 0:
		close(fd1[WRITE]);
		close(fd2[READ]);

		if (dup2(fd1[READ], 0) < 0) {
			err(EXIT_FAILURE, "error");
		}
		close(fd1[READ]);
		if (dup2(fd2[WRITE], 1) < 0) {
			err(EXIT_FAILURE, "error");
		}
		close(fd2[WRITE]);

		execl("/usr/bin/sha256sum", "sha256sum", NULL);
		exit(EXIT_FAILURE);
		break;

	case -1:
		err(EXIT_FAILURE, "error");
		break;
	}
}

int
exc_father(DIR * dir, char **argv2, int fd_wr)
{
	int dscr_f;
	long nr;
	char pth[MAX_PATH];
	char buf[BUFFER];
	struct dirent *p_dir;

	while ((p_dir = readdir(dir)) != NULL) {
		if (!check_ext(p_dir->d_name, ".txt")
		    || !check_fich_reg(p_dir->d_type)) {
			continue;
		}

		join_path_f(pth, argv2[1], p_dir->d_name);

		if ((dscr_f = open(pth, O_RDONLY)) < 0) {
			perror("error");
			return 0;
		}

		while ((nr = read(dscr_f, &buf, BUFFER)) != 0) {
			if (nr < 0) {
				perror("error");
				return 0;
			}

			if ((write(fd_wr, buf, nr)) != nr) {
				fprintf(stderr, "No se ha podido escribir\n");
				return 0;
			}
		}
		close(dscr_f);
	}
	closedir(dir);
	close(fd_wr);

	return 1;
}

int
clean_result(int fd_r)
{
	int nr;
	int status;
	char *save = NULL;
	char *p_result;
	char buf[BUFFER];

	switch (fork()) {
	case 0:
		while ((nr = read(fd_r, &buf, BUFFER)) != 0) {
			if (nr < 0) {
				perror("error");
				exit(EXIT_FAILURE);
			}
		}
		close(fd_r);
		p_result = strtok_r(buf, " ", &save);
		printf("%s\n", p_result);

		exit(EXIT_SUCCESS);
		break;

	case -1:
		err(EXIT_FAILURE, "error");
	}

	close(fd_r);
	wait(&status);

	return WEXITSTATUS(status);
}

int
main(int argc, char *argv[])
{
	int fd1[2];
	int fd2[2];
	DIR *dir;

	if (!check_n_args(argc)) {
		errx(EXIT_FAILURE, "usage: txtsha2 [dir]");
	}

	if (pipe(fd1) < 0) {
		err(EXIT_FAILURE, "error");
	}
	if (pipe(fd2) < 0) {
		err(EXIT_FAILURE, "error");
	}

	exc_sha(fd1, fd2);

	if (argc == 2) {
		dir = opendir(argv[1]);
	} else {
		dir = opendir(".");
	}

	if (!check_dir(dir)) {
		exit(EXIT_FAILURE);
	}
	close(fd1[READ]);
	close(fd2[WRITE]);

	//  The generated SHA-256 summary is 32 bytes, 
	//  so it does not imply a significant load for the parent, 
	//  so it is not necessary to create another child to read files.
	if (!exc_father(dir, argv, fd1[WRITE])) {
		exit(EXIT_FAILURE);
	}

	if (clean_result(fd2[READ])) {
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
