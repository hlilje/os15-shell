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

int pipe_exec_cmd(const char* cmd, int* pipes, const int* fds, char** args)
{
    pid_t pid;

	printf("pipe_exec_cmd %s: \n", cmd);
	if (args == NULL) printf("no args\n");

    /* Get file descriptors */
    if (pipe(pipes))
    {
        perror("Failed to create pipe");
        return 0;
    }

    /* Create new process */
    pid = fork();

    if (pid < 0)
    {
        perror("Failed to fork");
        return 0;
    }
    /* Child process */
    else if (pid == 0)
    {
        printf("CHILD\n");
        /* Copy and overwrite file descriptor */
        if (dup2(pipes[fds[0]], fds[1]) < 0)
        {
            perror("Failed to duplicate file descriptor for writing");
            return 0;
        }
        printf("CHILD AGAIN\n");

        /* Delete file descriptors */
        if (close(pipes[fds[0]]))
        {
            perror("Failed to delete file descriptor");
            return 0;
        }
        if (close(pipes[fds[1]]))
        {
            perror("Failed to delete file descriptor");
            return 0;
        }

        /* Execute command with arguments via path */
        if (args != NULL)
        {
            printf("EXEC CHILD\n");
            if (execvp("grep", args))
            {
                perror("Failed to execute command");
                return 0;
            }
        }
        /* Execute command without arguments via path */
        else
        {
            printf("EXEC CHILD\n");
            if (execlp(cmd, cmd, NULL))
            {
                perror("Failed to execute command");
                return 0;
            }
        }
    }

    return 1;
}

int check_env(const char* input, int i)
{
	int pipes[2], fds[2], status, j, k;
	int num_processes = 3;
	char* args[80];
    char* pager = getenv("PAGER");
	char cmd[80];

    j = 1;
    /* Read arguments to grep */
    while (input[i] != '\0')
    {
        i = read_cmd(cmd, input, i);
        args[j] = cmd;
        ++j;
    }

    /* Argument list is NULL terminated */
    args[j] = (char*) NULL;

    /* First argument must be file name */
    args[0] = cmd;

    /* DEBUG */
    printf("First argument: %s\n", args[1]);

    /* Pipe and execute printenv */
    fds[0] = WRITE;
    fds[1] = WRITE;
    if(!pipe_exec_cmd("printenv", pipes, fds, NULL))
    {
        perror("Failed to execute printenv");
        return 0;
    }

    /* Only pipe and excute grep if arguments were given */
    fds[0] = READ;
    fds[1] = READ;
    if (j > 1)
    {
        num_processes = 4;
        if (!pipe_exec_cmd("grep", pipes, fds, args))
        {
            perror("Failed to to execute grep");
            return 0;
        }
    }

    /* Pipe and execute sort */
    if (!pipe_exec_cmd("sort", pipes, fds, NULL))
    {
        perror("Failed to to execute sort");
        return 0;
    }

    /* Try to pipe and execute with PAGER environment variable */
    fds[0] = WRITE;
    fds[1] = WRITE;
    if (pager)
    {
        if (!pipe_exec_cmd(pager, pipes, fds, NULL))
        {
            perror("Failed to to execute checkEnv with environment pager");
            return 0;
        }
    }
    /* Try to pipe and execute with pager `less`, then `more` */
    else
    {
        if (!pipe_exec_cmd("less", pipes, fds, NULL))
        {
            if (!pipe_exec_cmd("more", pipes, fds, NULL))
            {
                perror("Failed to to execute checkEnv with default pagers");
                return 0;
            }
        }
    }

    /* Wait for all processes. */
    for (k = 0; k < num_processes; ++k)
    {
        /* Wait for the processes to finish */
        if (wait(&status) < 0)
        {
            perror("Failed to wait for process");
            return 0;
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
