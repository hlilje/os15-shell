#define _XOPEN_SOURCE 500
#define SIGDET 1
#define READ STDIN_FILENO
#define WRITE STDOUT_FILENO
#define ERROR STDERR_FILENO

#ifdef __APPLE__
#include <limits.h>
#else
#include <linux/limits.h>
#endif
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>


/**
 * Handle BG_TERM.
 */
void sig_bg_handler(const int sig);

/**
 * Print the prompt.
 * Return 0 upon failure and 1 upon success.
 */
int print_prompt();

/**
 * Read one command from input into cmd.
 * Return the new index i.
 */
int read_cmd(char* cmd, const char* input, int i);

/**
 * Exit the shell.
 */
void exit_shell();

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
 * Close the given number of pipes.
 * Return 0 upon failure and 1 upon success.
 */
int close_pipes(int* pipes, const int num_pipes);

/**
 * Fork and execute the given command.
 * If args is NULL, then no arguments will be given to the command.
 * File descriptors pairs in fds equalling -1 are not duped, the
 * first pair is for reading and the second for writing.
 * If try_less_more = 1, cmd is assumed to be less and more will be executed
 * if less fails.
 * Return 0 upon failure and 1 upon success.
 */
int fork_exec_cmd(const char* cmd, int* pipes, const int* fds, char** args,
        const int num_pipes, const int try_less_more);

/**
 * Check environment variables.
 * Return 0 upon failure and 1 upon success.
 */
int check_env(const char* input, int i);

/**
 * Execute arbitrary commands given to the shell.
 * Return 0 upon failure and 1 upon success.
 */
int general_cmd(char* input, const struct sigaction* act_int_old,
        const int* bg_pipes);

/**
 * Print accumulated process info.
 * Return 0 upon failure and 1 upon success.
 */
int print_process_info(const int* bg_pipes);
