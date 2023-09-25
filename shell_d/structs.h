enum {
	MAX_LINE = 256,
	MAX_CMD_F = 128,
	MAX_PATH = 512,
	READ = 0,
	WRITE = 1,
	MAX_HDOC = 1024 * 2
};

struct Cmd {
	int flag_hdoc;
	int flag_amp;
	int n_words;
	char **argv_cmd;
	char str_cmd[MAX_CMD_F];
};
typedef struct Cmd Cmd;

struct Redirs {
    int red_entr;
	int red_exit;
	int descr_f_entr;
	int descr_f_exit;
    char f_entr[MAX_LINE];
    char f_exit[MAX_LINE];
};
typedef struct Redirs Redirs;

struct Pipes {
    char **set_cmds;
	int **fds;
	int n_pipes;
	pid_t last_pid;
};
typedef struct Pipes Pipes;

struct VarPath {
	int n_dir_entpath;
	char cpy_vpath[MAX_LINE];
	char *var_entpath;
	char **div_entpath;
};
typedef struct VarPath VarPath;

struct HDoc {
	char doc[MAX_HDOC];
	int fd_hd[2];
};
typedef struct HDoc HDoc;
