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

int exit_shell()
{
    printf("exit\n");
    /* TODO Kill all children */
    exit(0);

    return 1;
}

int cd(const char* input, char* cmd, int i)
{
    /* Change to home directory */
    if (input[i] == '\0' || input[i] == '~')
    {
        char* home = getenv("HOME");
        if (!home)
        {
            perror("Failed to get home directory");
            return 0;
        }
        else if (chdir(home))
        {
            perror("Failed to change directory to HOME");
            return 0;
        }
    }
    /* Change to given directory */
    else
    {
        i = read_cmd(cmd, input, i);
        if (chdir(cmd))
        {
            perror("Failed to change directory");
            return 0;
        }
    }

    return 1;
}

int check_env(const char* input, int i)
{
    char checkenv[128];
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
        if (system(checkenv))
        {
            perror("Failed to to execute checkEnv with environment pager");
            return 0;
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
                return 0;
            }
        }
    }
    return 1;
}

int main(int argc, const char* argv[])
{
    while(1)
    {
        char input[80], cmd[80], wd[PATH_MAX];
        int i, j, do_fork;
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
        i = 0; /* Input index */
        i = read_cmd(cmd, input, i);

        if (strcmp(cmd, "exit") == 0)
        {
            if (!exit_shell()) break;
        }
        else if (strcmp(cmd, "cd") == 0)
        {
            if (!cd(input, cmd, i)) break;
        }
        else if (strcmp(cmd, "checkEnv") == 0)
        {
            if (!check_env(input, i))
                break;
        }
        else
        {
            /* Read the entire command string */
            do_fork = 0;
            for (j = 0; ; ++j)
            {
                /* Check if the process should run in the background */
                if (input[j] == '&')
                {
                    do_fork = 1;
                    input[j] = '\0';
                    break;
                }
                else if (input[j] == '\0')
                {
                    break;
                }
            }

            if (do_fork)
            {
                pid = fork(); /* Create new child process */

                /* Child process */
                if (pid == 0)
                {
                    if (system(input))
                    {
                        perror("Failed to execute forked command");
                    }

                    _exit(0); /* exit() unreliable */
                }
                /* Error */
                else if (pid < 0)
                {
                    perror("Failed to fork child process");
                    exit(1);
                }
                else
                {
                    /* The parent process comes here after forking */
                    /* int status; */
                    /* waitpid(pid, &status, 0); */
                }
            }
            /* Parent process */
            else
            {
                if (system(input))
                {
                    perror("Failed to execute command");
                }
            }
        }
    }

    return 0;
}
