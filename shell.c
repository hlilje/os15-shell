#include "shell.h"


int read_cmd(char* cmd, const char* input, int i)
{
    /* Read one command */
    int j;
    for (j = 0; ; ++i, ++j)
    {
        cmd[j] = input[i];
        /* TODO Check for escaped spaces */
        if (input[i] == ' ' || input[i] == '\0')
        {
            /* Let i be the next non-space character */
            while (input[i] == ' ') ++i;
            cmd[j] = '\0';
            break;
        }
    }

    return i;
}

int main(int argc, const char* argv[])
{
    while(1)
    {
        char input[80], cmd[80], buf[100];
        int i;

        printf("> "); /* Prompt */

        /* Exit if error occurs */
        if (!fgets(input, 80, stdin)) exit(1);

        /* Remove newline, if present */
        i = strlen(input) - 1;
        if (input[i] == '\n') input[i] = '\0';

        /* Read given commands */
        for(i = 0;;)
        {
            /* Read one command */
            i = read_cmd(cmd, input, i);

            if (strcmp(cmd, "exit") == 0)
            {
                printf("exit\n");
                /* TODO Kill all children */
                exit(0);
            }
            else if (strcmp(cmd, "cd") == 0)
            {
                printf("cd\n");
                printf("%s\n", getwd(buf));
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
