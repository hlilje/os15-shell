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
 * Create the given number of pipes.
 * Return 0 upon failure and 1 upon success.
 */
int create_pipes(int* pipes, const int num_pipes);

/**
 * Fork and execute the given command.
 * If args is NULL, then no arguments will be given to the command.
 * File descriptors pairs in fds equalling -1 are not duped, the
 * first pair is for reading and the second for writing.
 * If endpoint = 1, then no pipe will be created.
 * Return 0 upon failure and 1 upon success.
 */
int fork_exec_cmd(const char* cmd, int* pipes, const int* fds, char** args,
        const int num_pipes);

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
