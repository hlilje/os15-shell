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

int pipe_exec_cmd(const char* cmd, int* pipes, const int* fds, char** args,
        int num_pipes)
{
    pid_t pid;
    int i;

	printf("Execute command %s\n", cmd);

	printf("File descriptors %d, %d, %d, %d\n", fds[0], fds[1], fds[2], fds[3]);

	if (args == NULL) printf("No args for grep\n");

    /* Pipe and get file descriptors */
    if (pipe(pipes))
    {
        perror("Failed to create pipe");
        return 0;
    }

    /* Fork to create new process */
    pid = fork();

    if (pid < 0)
    {
        perror("Failed to fork");
        return 0;
    }
    /* Child process goes here, parent just returns */
    else if (pid == 0)
    {
        printf("Now executing in child process\n");
        /* Copy and overwrite file descriptor if set to do so */
        if (fds[0] != -1 && fds[1] != -1)
        {
            printf("Dup first fds\n");
            if (dup2(pipes[fds[0]], fds[1]) < 0)
            {
                perror("Failed to duplicate file descriptor for writing");
                return 0;
            }
        }
        if (fds[2] != -1 && fds[3] != -1)
        {
            printf("Dup second fds\n");
            if (dup2(pipes[fds[2]], fds[3]) < 0)
            {
                perror("Failed to duplicate file descriptor for writing");
                return 0;
            }
        }

        /* Delete all file descriptors for the child process */
        for (i = 0; i < num_pipes * 2; ++i)
        {
            if (close(pipes[i]))
            {
                perror("Failed to delete file descriptor");
                return 0;
            }
        }

        /* Execute command with arguments via path */
        if (args != NULL)
        {
            printf("Execute command with args\n");
            if (execvp("grep", args))
            {
                perror("Failed to execute command");
                return 0;
            }
        }
        /* Execute command without arguments via path */
        else
        {
            printf("Execute command w/o args\n");
            if (execlp(cmd, cmd, NULL))
            {
                perror("Failed to execute command");
                return 0;
            }
        }
    }

    printf("Now exiting in parent process\n");

    return 1;
}

int check_env(const char* input, int i)
{
	int pipes[8];                   /* File descriptors from piping */
	int fds[4];                     /* File descriptors to dupe */
	int status;                     /* Wait status */
	int j = 1;                      /* Loop index */
	int num_pipes = 3;              /* Number of pipes to create */
	int p;                          /* Offset in pipe array when piping */
	char* args[80];                 /* All arguments to grep */
    char* pager = getenv("PAGER");  /* PAGER enviroment variable */
	char cmd[80];                   /* One grep parameter */

	printf("Number of pipes: %i\n", num_pipes);

    /* Read arguments to grep */
    while (input[i] != '\0')
    {
        i = read_cmd(cmd, input, i);
        args[j] = cmd;
        ++j;
    }

    /* If arguments were give, one pipe is needed for grep */
    if (j > 1) num_pipes = 4;

    /* Argument list to execvp is NULL terminated */
    args[j] = (char*) NULL;

    /* First argument in list must be file name */
    args[0] = cmd;

    printf("First argument: %s\n", args[1]);

    /* Pipe and execute printenv */
    p = 0;
    fds[0] = 1;
    fds[1] = WRITE;
    fds[2] = -1;
    fds[3] = -1;
    if(!pipe_exec_cmd("printenv", pipes + p, fds, NULL, num_pipes))
    {
        perror("Failed to execute printenv");
        return 0;
    }

    /* Only pipe and excute grep if arguments were given */
    p += 2;
    fds[0] = 3;
    fds[1] = WRITE;
    fds[2] = 0;
    fds[3] = READ;
    if (num_pipes == 4)
    {
        if (!pipe_exec_cmd("grep", pipes + p, fds, args, num_pipes))
        {
            perror("Failed to to execute grep");
            return 0;
        }
    }

    /* Pipe and execute sort */
    if (num_pipes == 4)
    {
        p += 2;
        fds[0] = 5;
        fds[1] = WRITE;
        fds[2] = 2;
        fds[3] = READ;
    }
    if (!pipe_exec_cmd("sort", pipes + p, fds, NULL, num_pipes))
    {
        perror("Failed to to execute sort");
        return 0;
    }

    /* Try to pipe and execute with PAGER environment variable */
    p += 2;
    fds[0] = -1;
    fds[1] = -1;
    fds[2] = (num_pipes == 4) ? 4 : 2;
    fds[3] = READ;
    if (pager)
    {
        if (!pipe_exec_cmd(pager, pipes + p, fds, NULL, num_pipes))
        {
            perror("Failed to to execute checkEnv with environment pager");
            return 0;
        }
    }
    /* Try to pipe and execute with pager `less`, then `more` */
    /* TODO This might need more special care */
    else
    {
        if (!pipe_exec_cmd("less", pipes + p, fds, NULL, num_pipes))
        {
            if (!pipe_exec_cmd("more", pipes + p, fds, NULL, num_pipes))
            {
                perror("Failed to to execute checkEnv with default pagers");
                return 0;
            }
        }
    }

    /* Let the parent processes close all pipes */
    for (j = 0; j < num_pipes * 2; ++j)
    {
        printf("Close pid %d\n", pipes[j]);
        if (close(pipes[j]))
        {
            perror("Failed to delete file descriptor");
            return 0;
        }
    }

    /* Let the parent processes wait for all children */
    /* TODO Why +1? */
    for (j = 0; j < num_pipes + 1; ++j)
    {
        printf("Wait for process %d\n", pipes[j]);
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
