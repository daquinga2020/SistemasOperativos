#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "proc_txt.h"

int
n_wd(char *phrase, char *sep)
{
	int count = 0;
	char *sv;
	char *token;
	char copy[MAX_LINE];

	strncpy(copy, phrase, strlen(phrase) + 1);
	token = strtok_r(copy, sep, &sv);

	while (token != NULL) {
		count++;
		token = strtok_r(NULL, sep, &sv);
	}

	return count;
}

void
divide_words(char *phrase, char *sep, char **div_wd)
{
	int i;
	char *token;
	char *sv;

	token = strtok_r(phrase, sep, &sv);
	div_wd[0] = token;
	for (i = 1; token != NULL; i++) {
		token = strtok_r(NULL, sep, &sv);
		if (token != NULL) {
			div_wd[i] = token;
		}
	}
}

void
sv_redir(char *line, char red, char *svfile, int *flag_red)
{
	char delim2[] = ">< ";
	char *sv;
	char *last_ch;
	char *token;

	last_ch = strrchr(line, red);

	if (last_ch == NULL) {
		*flag_red = 0;
	} else {
		*flag_red = 1;
		token = strtok_r(last_ch, delim2, &sv);
		strncpy(svfile, token, strlen(token) + 1);
	}
}

int
identify_elem(char *line_cmds, char elem)
{
	int i;
	int n_elem = 0;

	for (i = 0; i < strlen(line_cmds); i++) {
		if (line_cmds[i] == elem) {
			n_elem++;
		}
	}

	return n_elem;
}

int
find_amps(char *line_cmd)
{
	char copy[MAX_LINE];
	char *last_ch;
	int n = 0;

	strncpy(copy, line_cmd, strlen(line_cmd) + 1);
	last_ch = strrchr(copy, '&');

	if (last_ch != NULL) {
		n = n_wd(last_ch, "& ");

		if (n != 0) {
			n = -1;
		} else {
			n = 1;
		}
	}
	return n;
}

int
exist_letter(char *wd)
{
	int i;

	for (i = 0; i < strlen(wd); i++) {
		if (wd[i] != ' ') {
			return 1;
		}
	}
	return 0;
}

int
check_setpipes(Pipes * p)
{
	int i;

	for (i = 0; i < (p->n_pipes + 1); i++) {
		if (p->set_cmds[i] == NULL) {
			return 0;
		} else if (!exist_letter(p->set_cmds[i])) {
			return 0;
		}
	}
	return 1;
}

int
join_path_cmd(char *path, Cmd * cmd)
{
	int sz;
	int n1;

	sz = strlen(cmd->argv_cmd[0]) + strlen(path) + 2;
	n1 = snprintf(cmd->str_cmd, sz, "%s/%s", path, cmd->argv_cmd[0]);
	if (n1 > sz || n1 < 0) {
		fprintf(stderr, "error processing %s%s\n", path,
			cmd->argv_cmd[0]);
		return 0;
	}
	return 1;
}

int
check_cmd_setvar(char *wd)
{
	int i;

	for (i = 0; i < strlen(wd); i++) {
		if (wd[i] == '=' && wd[i - 1] != ' ' && wd[i + 1] != ' ') {
			return 1;
		}
	}

	return 0;
}

int
find_letter(char *wd, char letter)
{
	int i;

	for (i = 0; i < strlen(wd); i++) {
		if (wd[i] == letter) {
			return i;
		}
	}
	return -1;
}

int
obtain_char(char *phrase)
{
	int i;
	char special_c[6] = { '$', '|', ' ', '<', '>', '&' };
	int index;
	int min = -1;
	char c = '\0';

	for (i = 0; i < 6; i++) {
		index = find_letter(phrase, special_c[i]);

		if (index != -1 && (index < min || min == -1)) {
			min = index;
			c = special_c[i];
		}
	}

	return c;
}

int
replace_var(int n_var, char *phrase)
{
	int i;
	char *var;
	char *content;
	char *before_v;
	char *sv1;
	char *sv2;
	char copy[MAX_LINE];
	char c;

	for (i = 0; i < n_var; i++) {
		before_v = strtok_r(phrase, "$", &sv1);

		if (before_v != NULL && strlen(sv1)) {
			c = obtain_char(sv1);
			var = strtok_r(sv1, " $<>|&", &sv2);
			if ((content = getenv(var)) == NULL) {
				fprintf(stderr,
					"error: var %s does not exist\n", var);
				return EXIT_FAILURE;
			}
			snprintf(copy, MAX_LINE, "%s%s%c%s", phrase, content, c,
				 sv2);
			strncpy(phrase, copy, strlen(copy) + 1);
		}
	}
	return EXIT_SUCCESS;
}
