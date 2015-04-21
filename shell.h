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


/**
 * Read one command from input into cmd.
 * Return the new index i.
 */
int read_cmd(char* cmd, const char* input, int i);

/**
 * Exit the shell.
 */
int exit_shell();

/**
 * Change directory.
 * Return 0 upon failure and 1 upon success.
 */
int cd(const char* input, char* cmd, int i);

/**
 * Check environment variables
 */
int checkEnv(char* input, int i);
