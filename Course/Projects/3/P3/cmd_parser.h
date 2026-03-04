
#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#define CMD_OK 0
#define CMD_EINVAL 1
#define CMD_E2BIG 2
#define CMD_ENOENT 3

#define MAXARGS 10
#define MAXCMDLINE 256

/* main loop - reads input and dispatches commands */
void run_command_loop(void);

/* individual command handlers */
int cmd_help(int nargs, char **args);
int cmd_run(int nargs, char **args);
int cmd_list(int nargs, char **args);
int cmd_fcfs(int nargs, char **args);
int cmd_sjf(int nargs, char **args);
int cmd_priority(int nargs, char **args);
int cmd_test(int nargs, char **args);
int cmd_quit(int nargs, char **args);

#endif
