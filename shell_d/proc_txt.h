int n_wd(char *phrase, char *sep);

void divide_words(char *phrase, char *sep, char **div_wd);

void sv_redir(char *line, char red, char *svfile, int *flag_red);

int identify_elem(char *line_cmds, char elem);

int find_amps(char *line_cmd);

int exist_letter(char *wd);

int check_setpipes(Pipes * p);

int join_path_cmd(char *path, Cmd * cmd);

int check_cmd_setvar(char *wd);

int find_letter(char *wd, char letter);

int obtain_char(char *phrase);

int replace_var(int n_var, char *phrase);