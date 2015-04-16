#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"

int main(int argc, const char* argv[])
{
    for (;;)
    {
        char input[80], cmd[80];
        int i, j;

        printf("> "); /* Prompt */

        fgets(input, 80, stdin);

        /* Remove newline, if present */
        i = strlen(input) - 1;
        if (input[i] == '\n') input[i] = '\0';

        /* Read given commands */
        for (i = 0;; ++i, ++j) /* Increment to skip space */
        {
            /* Read one command */
            for (j = 0; ; ++i, ++j)
            {
                cmd[j] = input[i];
                if (input[i] == ' ' || input[i] == '\0')
                {
                    cmd[j] = '\0';
                    break;
                }
            }

            if (strcmp(cmd, "exit") == 0)
            {
                printf("exit\n");
                /* TODO Kill all children */
                exit(0);
            }
            else if (strcmp(cmd, "cd") == 0)
            {
                printf("cd\n");
            }
            else if (strcmp(cmd, "checkEnv") == 0)
            {
                printf("checkEnv\n");
            }
            else
            {
                printf("else\n");
            }

            if (input[i] == '\0') break;
        }
    }

    return 0;
}
