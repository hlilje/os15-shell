#ifdef __APPLE__
#include <limits.h>
#else
#include <linux/limits.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define READ  0
#define WRITE 1


/**
 * Read one command from input into cmd.
 * Return the new index i.
 */
int read_cmd(char* cmd, const char* input, int i);

/**
 * Exit the shell.
 * Return 0 upon failure and 1 upon success.
 */
int exit_shell();

/**
 * Change directory.
 * Return 0 upon failure and 1 upon success.
 */
int cd(const char* input, char* cmd, int i);

/**
 * Pipe and execute the given command.
 * If args is NULL, then no arguments will be given to the command.
 * Return 0 upon failure and 1 upon success.
 */
int pipe_exec_cmd(const char* cmd, int* pipes, const int* fds, char** args);

/**
 * Check environment variables
 * Return 0 upon failure and 1 upon success.
 */
int check_env(const char* input, int i);

/**
 * Execute arbitrary commands given to the shell.
 * Return 0 upon failure and 1 upon success.
 */
int general_cmd(char* input);
