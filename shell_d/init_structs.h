void init_redirs(Redirs * rs, char *line_cmd);

void init_varpath(VarPath * vp);

void init_pipes(Pipes * p, char *line_cmd);

void init_cmd(Cmd * cmd, char *line_cmd, char *sep, Redirs * rs, int last);

int init_hdoc(HDoc *hd);