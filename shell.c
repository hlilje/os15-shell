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
    /* TODO Remove */
    char checkenv[128], checkenvtmp[128];
    char* pager = getenv("PAGER");

    pid_t pid_printenv, pid_grep, pid_sort, pid_pager;
	int pipes1[2], pipes2[2], pipes3[2], status, j, do_grep;
	char* args[80];
	char cmd[80];

    /* First argument must be file name */
	args[0] = "grep";
	j = 1;
	do_grep = 0;
	/* Read arguments to grep */
	while (input[i] != '\0')
    {
        i = read_cmd(cmd, input, i);
        args[j] = cmd;
        ++j;
    }

    /* Argument list is NULL terminated */
    args[j] = (char*) NULL;

	/* Get file descriptors */
	if (pipe(pipes1))
    {
        perror("Failed to create pipe for printenv -> grep");
        return 0;
    }
	if (pipe(pipes2))
    {
        perror("Failed to create pipe for -> sort");
        return 0;
    }
	if (pipe(pipes3))
    {
        perror("Failed to create pipe for sort -> pager");
        return 0;
    }

    /* Create new printenv process */
    pid_printenv = fork();

    if (pid_printenv < 0)
    {
        perror("Failed to fork for printenv");
        return 0;
    }
    /* Child process */
    else if (pid_printenv == 0)
    {
        /* Copy and overwrite file descriptor */
        if (dup2(pipes1[WRITE], WRITE) < 0)
        {
            perror("Failed to duplicate file descriptor for writing");
            return 0;
        }

        /* Delete file descriptors */
        if (close(pipes1[WRITE]))
        {
            perror("Failed to delete file descriptor");
            return 0;
        }
        if (close(pipes1[READ]))
        {
            perror("Failed to delete file descriptor");
            return 0;
        }

        /* Execute printenv via path */
        if (execlp("printenv", "printenv", NULL))
        {
            perror("Failed to execute printenv");
            return 0;
        }
    }

    /* Create new grep process if arguments were given */
    if (args[1] != NULL)
    {
        do_grep = 1;

        pid_grep = fork();
        if (pid_grep < 0)
        {
            perror("Failed to create pipe for grep");
            return 0;
        }
        /* Child process */
        else if (pid_grep == 0)
        {
            /* Copy and overwrite file descriptor */
            if (dup2(pipes1[READ], READ) < 0)
            {
                perror("Failed to duplicate file descriptor for reading");
                return 0;
            }

            /* Delete file descriptors */
            if (close(pipes1[WRITE]))
            {
                perror("Failed to delete file descriptor");
                return 0;
            }
            if (close(pipes1[READ]))
            {
                perror("Failed to delete file descriptor");
                return 0;
            }

            /* Execute grep via path using given arguments */
            if (execvp("grep", args))
            {
                perror("Failed to execute grep");
                return 0;
            }
        }
    }

    /* Create new sort process */
    pid_sort = fork();
    if (pid_sort < 0)
    {
        perror("Failed to create pipe for sort");
        return 0;
    }
    /* Child process */
    else if (pid_sort == 0)
    {
        /* Copy and overwrite file descriptor */
        if (dup2(pipes2[READ], READ) < 0)
        {
            perror("Failed to duplicate file descriptor for reading");
            return 0;
        }

        /* Delete file descriptors */
        if (close(pipes2[WRITE]))
        {
            perror("Failed to delete file descriptor");
            return 0;
        }
        if (close(pipes2[READ]))
        {
            perror("Failed to delete file descriptor");
            return 0;
        }

        /* Execute sort via path */
        if (execlp("sort", "sort", NULL))
        {
            perror("Failed to execute sort");
            return 0;
        }
    }

    /* Wait for the processes to finish */
    if (wait(&status) < 0)
    {
        perror("Failed to wait for first process");
        return 0;
    }
    /* Only wait if grep has been executed */
    if (do_grep && wait(&status) < 0)
    {
        perror("Failed to wait for second process");
        return 0;
    }
    if (wait(&status) < 0)
    {
        perror("Failed to wait for the third process");
        return 0;
    }

	return 1;

	/* TODO Remove */

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
        strcpy(checkenvtmp, checkenv);
        strcat(checkenv, "less");
        if (system(checkenv))
        {
            strcat(checkenvtmp, "more");
            if (system(checkenvtmp))
            {
                perror("Failed to to execute checkEnv with default pagers");
                return 0;
            }
        }
    }

    return 1;
}

int general_cmd(char* input)
{
    int i, do_fork;
    pid_t pid;

    /* Read the entire command string */
    do_fork = 0;
    for (i = 0; ; ++i)
    {
        /* Check if the process should run in the background */
        if (input[i] == '&')
        {
            do_fork = 1;
            input[i] = '\0';
            break;
        }
        else if (input[i] == '\0')
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

    return 1;
}

int main(int argc, const char* argv[])
{
    while(1)
    {
        char input[80], cmd[80], wd[PATH_MAX];
        int i;

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
            if (!check_env(input, i)) break;
        }
        else
        {
            if (!general_cmd(input)) break;
        }
    }

    return 0;
}
