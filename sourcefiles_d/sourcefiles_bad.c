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
	MAX_PATH = 124
};

struct Files {
	int error;
	int ext_c;
	int ext_h;
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

Files
process_dir(DIR * dir, char *path)
{
	char new_path[MAX_PATH * 3];

	int n_files_c = 0;
	int n_files_h = 0;
	long int size_tot = 0;

	struct dirent *p_dir;
	struct stat st;
	DIR *subdir;
	Files file;
	Files file_aux;

	file.error = 0;
	while ((p_dir = readdir(dir)) != NULL) {
		snprintf(new_path, sizeof(new_path), "%s/%s", path,
			 p_dir->d_name);

		if (check_dir(p_dir->d_type) && strcmp(p_dir->d_name, ".") != 0
		    && strcmp(p_dir->d_name, "..") != 0) {

			subdir = opendir(new_path);// falta comprobar si fallas

			file_aux = process_dir(subdir, new_path);

			n_files_c = n_files_c + file_aux.n_files_c;
			n_files_h = n_files_h + file_aux.n_files_h;
			size_tot = size_tot + file_aux.size_tot_by;
			
			continue;
		} else if (check_fich_reg(p_dir->d_type)) {
			file.ext_c = check_ext(p_dir->d_name, 'c');
			file.ext_h = check_ext(p_dir->d_name, 'h');
		} else {
			continue;
		}
		
		if (stat(new_path, &st) < 0) {
			perror("error");
			file.error = 1;
			break;
		}
		
		if (file.ext_c) {
			n_files_c++;
		} else if (file.ext_h) {
			n_files_h++;
		} else {
			continue;
		}
		size_tot = size_tot + st.st_size;
	}
	file.n_files_c = n_files_c;
	file.n_files_h = n_files_h;
	file.size_tot_by = size_tot;

	closedir(dir);
	
	return file;
}

int
main(int argc, char *argv[])
{
	int type_exit = EXIT_SUCCESS;
	int sv_err_dir = 0;

	char path_arg[MAX_PATH];
	char path_current[MAX_PATH];
	char path_abs[MAX_PATH * 2];

	DIR *dir;
	Files f;

	if (!check_n_args(argc)) {
		errx(EXIT_FAILURE, "usage: sourcefiles path");
	}

	while (fgets(path_arg, MAX_PATH, stdin) != NULL) {
		if (path_arg[strlen(path_arg) - 1] == '\n') {
			path_arg[strlen(path_arg) - 1] = '\0';	// To elimnate newline
		}
		if (path_arg[0] != '/') {
			getcwd(path_current, MAX_PATH);
			snprintf(path_abs, sizeof(path_abs), "%s/%s",
				 path_current, path_arg);
		} else {
			strncpy(path_abs, path_arg, MAX_PATH);
		}

		dir = opendir(path_abs);
		if (dir == NULL) {
			sv_err_dir = errno;
			if (sv_err_dir == ENOENT) {
				fprintf(stderr, "usage: sourcefiles path\n");
				type_exit = EXIT_FAILURE;
				continue;
			} else {
				perror("error");
			}
		}

		f = process_dir(dir, path_abs);
		if (!f.error) {
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
