#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <err.h>

enum {
	MAX_PATH = 256
};

struct Files {
	int error;
	int n_files_c;
	int n_files_h;
	long int size_tot_by;
};

typedef struct Files Files;

int
check_fich_reg(unsigned char type)
{
	return type == DT_REG;
}

int
check_dir(unsigned char type)
{
	return type == DT_DIR;
}

int
check_n_args(int n_args)
{
	return n_args == 1;
}

int
check_ext(char *file, char typ_ext)
{
	char *last_ch = strrchr(file, '.');

	if (last_ch == NULL) {
		return 0;
	}

	return last_ch[1] == typ_ext && last_ch[2] == '\0';
}

int
process_file(Files * file, char *file_name)
{
	struct stat st;

	if (stat(file_name, &st) < 0) {
		perror("error");
		file->error = 1;
		return 0;
	}

	if (check_ext(file_name, 'c')) {
		file->n_files_c++;
	} else if (check_ext(file_name, 'h')) {
		file->n_files_h++;
	} else {
		return 1;
	}
	file->size_tot_by = file->size_tot_by + st.st_size;

	return 1;
}

int
process_dir(char *path, Files * file)
{
	char new_path[MAX_PATH * 3];
	int sv_err_dir;
	int n;

	struct dirent *p_dir;
	DIR *dir;

	if ((dir = opendir(path)) == NULL) {
		sv_err_dir = errno;
		if (sv_err_dir == ENOENT) {
			fprintf(stderr, "usage: sourcefiles path\n");
		} else {
			perror("error");
		}
		return file->error = 1;
	}

	while ((p_dir = readdir(dir)) != NULL) {
		n = snprintf(new_path, MAX_PATH * 3, "%s/%s", path,
			     p_dir->d_name);

		if (n > MAX_PATH * 3 || n < 0) {
			fprintf(stderr, "error processing %s/%s", path,
				p_dir->d_name);
			file->error = 1;
			break;
		}

		if (check_dir(p_dir->d_type) && strcmp(p_dir->d_name, ".") != 0
		    && strcmp(p_dir->d_name, "..") != 0) {

			process_dir(new_path, file);
			continue;
		} else if (check_fich_reg(p_dir->d_type)) {
			if (!process_file(file, new_path)) {
				break;
			}
		}
	}
	closedir(dir);

	return file->error;
}

int
main(int argc, char *argv[])
{
	int type_exit = EXIT_SUCCESS;
	char path_arg[MAX_PATH];
	char path_current[MAX_PATH];
	char path_abs[MAX_PATH * 2];

	Files f;

	if (!check_n_args(argc)) {
		errx(EXIT_FAILURE, "usage: sourcefiles path");
	}

	while (fgets(path_arg, MAX_PATH, stdin) != NULL) {
		if (path_arg[strlen(path_arg) - 1] == '\n') {
			path_arg[strlen(path_arg) - 1] = '\0';	// To delete newline
		}

		if (path_arg[0] != '/') {
			getcwd(path_current, MAX_PATH);
			snprintf(path_abs, sizeof(path_abs), "%s/%s",
				 path_current, path_arg);
		} else {
			strncpy(path_abs, path_arg, MAX_PATH);
		}

		f.n_files_c = 0;
		f.n_files_h = 0;
		f.size_tot_by = 0;
		f.error = 0;

		if (!process_dir(path_abs, &f)) {
			printf("%s\t%d\t%d\t%ld\n", path_arg, f.n_files_c,
			       f.n_files_h, f.size_tot_by);
		} else {
			type_exit = EXIT_FAILURE;
		}
	}
	if (!feof(stdin)) {
		type_exit = EXIT_FAILURE;
		fprintf(stderr, "eof not reached");
	}

	exit(type_exit);
}
