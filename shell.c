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
        char input[80], cmd[80], wd[PATH_MAX], checkenv[128];
        int i, status;
        pid_t pid;

        /* Prompt */
        if (!getcwd(wd, PATH_MAX))
        {
            perror("Failed to get current working directory");
            exit(1);
        }
        printf("%s", wd);
        printf(" > ");

        /* Exit if error occurs */
        if (!fgets(input, 80, stdin))
        {
            perror("Failed to get input");
            continue;
        }

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
                /* Change to home directory */
                if (input[i] == '\0' || input[i] == '~')
                {
                    char* home = getenv("HOME");
                    if (!home)
                    {
                        perror("Failed to get home directory");
                        break;
                    }
                    else if (chdir(home))
                    {
                        perror("Failed to change directory to HOME");
                        break;
                    }
                }
                /* Change to given directory */
                else
                {
                    i = read_cmd(cmd, input, i);
                    if (chdir(cmd))
                    {
                        perror("Failed to change directory");
                        break;
                    }
                }
            }
            else if (strcmp(cmd, "checkEnv") == 0)
            {
                char* pager = getenv("PAGER");
                strcpy(checkenv, "printenv");
                /* Get all given arguments */
                if (input[i] != '\0')
                {
                    strcat(checkenv, " | grep ");
                    strcat(checkenv, &input[i]);
                }
                strcat(checkenv, " | sort | ");
                /* Try to execute with PAGER environment variable */
                if (pager)
                {
                    strcat(checkenv, pager);
                    if(system(checkenv))
                    {
                        perror("Failed to to execute checkEnv with environment pager");
                        break;
                    }
                }
                /* Try to execute with pager `less`, then `more` */
                else
                {
                    char checkenvtmp[128];
                    strcpy(checkenvtmp, checkenv);
                    strcat(checkenv, "less");
                    if(system(checkenv))
                    {
                        strcat(checkenvtmp, "more");
                        if(system(checkenvtmp))
                        {
                            perror("Failed to to execute checkEnv with default pagers");
                            break;
                        }
                    }
                }
            }
            else
            {
                printf("else\n");
                i = read_cmd(cmd, input, i);
                printf("%s\n", cmd);

                pid = fork();

                /* Child process */
                if (pid == 0)
                {
                    printf("Child\n");
                    execl("/bin/ls", "ls", NULL);
                    _exit(0); /* exit() unreliable */
                }
                /* Error */
                else if (pid < 0)
                {
                    perror("Failed to fork child process");
                    exit(1);
                }
                /* Parent process */
                else
                {
                    printf("Parent\n");
                    /* Wait for child process to finish */
                    waitpid(pid, &status, 0);
                }
            }

            if (input[i] == '\0') break;
        }
    }

    return 0;
}
