int open_redirs(Redirs * rs, int flag_amps);

void apply_redirs(int flag_redir, int fd1, int fd2);

void proc_son(int index, Pipes * p, Redirs * rs);

void proc_fth(int index, Pipes * p, Redirs * rs, int fd_hd, int flag_amps, int flag_hd);

int exec_cmds(Pipes * p, Cmd * cmds, Redirs * rs, HDoc * hd);

int exec_cd(Cmd * cmd);

void free_cmd(Cmd * cmds, int n);

void free_pipes(Pipes * p);

int set_var(char *line, Cmd * cmd, Redirs * rs);

int wr_heredoc(HDoc * hd);

int setresult(int result);

int exec_ifbtn(char *line);