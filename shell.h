#ifdef __APPLE__
#include <limits.h>
#else
#include <linux/limits.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

/* Needed for signals */
#define _XOPEN_SOURCE 500
/* Handle background processes by signals */
#define SIGDET 1

#define READ  0
#define WRITE 1
#define BG_TERM 42

/**
 * Handle SIGINT.
 */
void sig_int_handler(const int sig);

/**
 * Handle EG_TERM.
 */
void sig_bg_handler(const int sig);

/**
 * Print the prompt.
 * Return 0 upon failure and 1 upon success.
 */
const int print_prompt();

/**
 * Read one command from input into cmd.
 * Return the new index i.
 */
const int read_cmd(char* cmd, const char* input, int i);

/**
 * Exit the shell.
 */
void exit_shell();

/**
 * Change directory.
 * Return 0 upon failure and 1 upon success.
 */
const int cd(const char* input, char* cmd, int i);

/**
 * Create the given number of pipes.
 * Return 0 upon failure and 1 upon success.
 */
const int create_pipes(int* pipes, const int num_pipes);

/**
 * Close the given number of pipes.
 * Return 0 upon failure and 1 upon success.
 */
const int close_pipes(int* pipes, const int num_pipes);

/**
 * Fork and execute the given command.
 * If args is NULL, then no arguments will be given to the command.
 * File descriptors pairs in fds equalling -1 are not duped, the
 * first pair is for reading and the second for writing.
 * If try_less_more = 1, cmd is assumed to be less and more will be executed
 * if less fails.
 * Return 0 upon failure and 1 upon success.
 */
const int fork_exec_cmd(const char* cmd, int* pipes, const int* fds, char** args,
        const int num_pipes, const int try_less_more);

/**
 * Check environment variables
 * Return 0 upon failure and 1 upon success.
 */
const int check_env(const char* input, int i);

/**
 * Execute arbitrary commands given to the shell.
 * Return 0 upon failure and 1 upon success.
 */
const int general_cmd(char* input);
