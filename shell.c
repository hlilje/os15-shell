#include "shell.h"

#if (SIGDET == 1)
const static int SIGN_DETECTION = 1;
#else
const static int SIGN_DETECTION = 0;
#endif


void sig_handler(const int sig)
{
    /* Ignore the signal */
    signal(sig, SIG_IGN);
}

int print_prompt()
{
    char wd[PATH_MAX];

    if (!getcwd(wd, PATH_MAX))
    {
        perror("Failed to get current working directory");
        return 0;
    }
    printf("%s", wd);
    printf(" > ");
    return 1;
}

int read_cmd(char* cmd, const char* input, int i)
{
    int j;
    /* Discard spaces */
    while (input[i] == ' ')
        i++;

    /* Read one command */
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

void exit_shell()
{
    /* TODO Kill all children */

    /* Kill all processes of the same group */
    /* TODO is this enough? */
    kill(0, SIGKILL);
    exit(0);
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

int create_pipes(int* pipes, const int num_pipes)
{
    int i, j;
    /* Pipe and get file descriptors */
    /* 1st ix = read, 2nd ix = write */
    for (i = 0, j = 0; i < num_pipes * 2; i += 2, ++j)
    {
        printf("Creating pipe %d\n", j);
        if (pipe(pipes + i))
        {
            perror("Failed to create pipe");
            return 0;
        }
    }

    return 1;
}

int close_pipes(int* pipes, const int num_pipes)
{
    int i;
    for (i = 0; i < num_pipes * 2; ++i)
    {
        printf("Now closing %d\n", pipes[i]);
        if (close(pipes[i]))
        {
            perror("Failed to delete file descriptor");
            return 0;
        }
    }

    return 1;
}

int fork_exec_cmd(const char* cmd, int* pipes, const int* fds, char** args,
        const int num_pipes, const int try_less_more)
{
    pid_t pid;
    int i;

    if (args == NULL)
    {
        printf("Execute command %s with fds %d, %d, %d, %d (no args)\n",
                cmd, fds[0], fds[1], fds[2], fds[3]);
    }
    else
    {
        printf("Execute command %s with fds %d, %d, %d, %d (args)\n", cmd,
                fds[0], fds[1], fds[2], fds[3]);
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
        printf("CHILD: Now executing in child process\n");
        /* Copy and overwrite file descriptor if set to do so */
        if (fds[0] != -1 && fds[1] != -1)
        {
            printf("CHILD: Dup fds for READ %d, %d\n", fds[0], fds[1]);
            if (dup2(pipes[fds[0]], fds[1]) < 0)
            {
                perror("Failed to duplicate file descriptor for writing");
                _exit(1);
            }
        }
        if (fds[2] != -1 && fds[3] != -1)
        {
            printf("CHILD: Dup fds for WRITE %d, %d\n", fds[2], fds[3]);
            if (dup2(pipes[fds[2]], fds[3]) < 0)
            {
                perror("Failed to duplicate file descriptor for writing");
                _exit(1);
            }
        }

        /* Delete all file descriptors for the child process */
        for (i = 0; i < num_pipes * 2; ++i)
        {
            printf("CHILD: Now closing %i\n", pipes[i]);
            if (close(pipes[i]))
            {
                perror("Failed to delete file descriptor");
                _exit(1);
            }
        }

        /* Execute command with arguments via path */
        if (args != NULL)
        {
            printf("CHILD: Execute command with args\n");
            if (execvp(cmd, args))
            {
                perror("Failed to execute command");
                _exit(1);
            }
        }
        /* Execute command without arguments via path */
        else
        {
            printf("CHILD: Execute command w/o args\n");
            /* Special case to try more if less fails */
            if (try_less_more)
            {
                if (execlp("less", cmd, NULL))
                {
                    printf("CHILD: Failed to execute less\n");
                    if (execlp("more", cmd, NULL))
                    {
                        perror("Failed to execute command");
                        _exit(1);
                    }
                }
            }
            else
            {
                if (execlp(cmd, cmd, NULL))
                {
                    perror("Failed to execute command");
                    _exit(1);
                }
            }
        }
    }

    printf("PARENT: Now exiting fork_exec_cmd\n");

    return 1;
}

int check_env(const char* input, int i)
{
    int pipes[6];                   /* File descriptors from piping */
    int fds[4];                     /* File descriptors to dupe */
    int status;                     /* Wait status */
    int j = 1;                      /* Loop index */
    int num_pipes = 2;              /* Number of pipes to create */
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

    /* If arguments were given, one pipe is needed for grep */
    if (j > 1) num_pipes = 3;

    /* Create all pipes beforehand */
    create_pipes(pipes, num_pipes);

    /* Argument list to execvp is NULL terminated */
    args[j] = (char*) NULL;

    /* First argument in list must be file name */
    args[0] = cmd;

    printf("First grep argument: %s\n", args[1]);

    /* pipe fds: (0, 1) [2, 3, 4, 5, 6, 7] */

    /* PIPE READ WRITE */
    /* 1    2    3     */
    /* 2    4    5     */
    /* 3    6    7     */

    /* PROC READ WRITE */
    /* 1    0    3     */
    /* 2    2    5     */
    /* 3    4    7     */
    /* 4    6    1     */

    /* Pipe and execute printenv */
    fds[0] = -1;
    fds[1] = -1;
    fds[2] = 1;
    fds[3] = WRITE;
    if(!fork_exec_cmd("printenv", pipes, fds, NULL, num_pipes, 0))
    {
        perror("Failed to execute printenv");
        return 0;
    }

    /* Only pipe and excute grep if arguments were given */
    fds[0] = 0;
    fds[1] = READ;
    fds[2] = 3;
    fds[3] = WRITE;
    if (num_pipes == 3)
    {
        if (!fork_exec_cmd("grep", pipes, fds, args, num_pipes, 0))
        {
            perror("Failed to to execute grep");
            return 0;
        }
    }

    /* Pipe and execute sort */
    if (num_pipes == 3)
    {
        fds[0] = 2;
        fds[1] = READ;
        fds[2] = 5;
        fds[3] = WRITE;
    }
    if (!fork_exec_cmd("sort", pipes, fds, NULL, num_pipes, 0))
    {
        perror("Failed to to execute sort");
        return 0;
    }

    /* Try to pipe and execute with PAGER environment variable */
    fds[0] = (num_pipes == 3) ? 4 : 2;
    fds[1] = READ;
    fds[2] = -1;
    fds[3] = -1;
    if (pager)
    {
        printf("Trying to use PAGER\n");
        if (!fork_exec_cmd(pager, pipes, fds, NULL, num_pipes, 0))
        {
            perror("Failed to to execute checkEnv with environment pager");
            return 0;
        }
    }
    /* Try to pipe and execute with pager `less`, then `more` */
    else
    {
        printf("Trying to use less or more\n");
        if (!fork_exec_cmd("more", pipes, fds, NULL, num_pipes, 1))
        {
            perror("Failed to to execute checkEnv with default pagers");
            return 0;
        }
    }

    /* Let the parent processes close all pipes */
    close_pipes(pipes, num_pipes);

    /* Let the parent processes wait for all children */
    for (j = 0; j < num_pipes + 1; ++j)
    {
        printf("PARENT: Wait for process %d\n", j);
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
    int background_process;
    int pipes[2];                   /* File descriptors from piping */
    int fds[4];                     /* File descriptors to dupe */
    int num_pipes = 1;              /* Number of pipes to create */
    pid_t pid;                      /* PID of child */
    int status;                     /* Wait status */
    char* args[80];                 /* All arguments to the command */
    char arg[80];                   /* One argument to command */
    char cmd[80];                   /* The command to be executed */
    int j = 1;                      /* Loop index */
    int i;                          /* Command index */
    time_t time_before;             /* Time before execution of command */
    time_t time_after;              /* Time after execution of command */

    create_pipes(pipes, num_pipes);
    fds[0] = -1;
    fds[1] = -1;
    fds[2] = -1;
    fds[3] = -1;

    /* Read the entire command string */
    background_process = 0;
    for (i = 0; ; ++i)
    {
        /* Check if the process should run in the background */
        if (input[i] == '&')
        {
            background_process = 1;
            input[i] = '\0';
            break;
        }
        else if (input[i] == '\0')
        {
            break;
        }
    }

    /* Read the command */
    i = read_cmd(cmd, input, 0);
    /* First argument in list must be file name */
    args[0] = cmd;

    /* Read arguments to the command */
    while (input[i] != '\0')
    {
        i = read_cmd(arg, input, i);
        args[j] = arg;
        ++j;
    }
    /* Argument list to execvp is NULL terminated */
    args[j] = (char*) NULL;

    pid = fork(); /* Create new child process that handles execution */

    /* Child process */
    if (pid == 0)
    {
        time(&time_before);
        if (!fork_exec_cmd(cmd, pipes, fds, args, num_pipes, 0))
        {
            perror("Failed to execute command");
            return 0;
        }
        if (wait(&status) < 0)
        {
            perror("Failed to wait for executing process");
            return 0;
        }
        time(&time_after);
        time_after = time_after - time_before;
        printf("%s finished executing in %ld seconds\n", cmd, time_after);
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
        /* The parent process comes here after forking */
        /* int status; */
        /* waitpid(pid, &status, 0); */
        if (!background_process)
        {
            if (waitpid(pid, &status, 0) < 0)
            {
                perror("Failed to wait for process");
                return 0;
            }
        }
    }

    /* Let the parent processes close all pipes */
    close_pipes(pipes, num_pipes);

    return 1;
}

int main(int argc, const char* argv[])
{
    int status;
    /* Define handler for SIGINT */
    signal(SIGINT, sig_handler);

    while(1)
    {
        char input[80], cmd[80];
        int i;

        /* Wait for all defunct children */
        /* Continue even if no child has exited */
        while(waitpid(-1, &status, WNOHANG | WUNTRACED) > 0);
        /* Prompt */
        if (!print_prompt()) continue;

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
            exit_shell();
        else if (strcmp(cmd, "cd") == 0)
            cd(input, cmd, i);
        else if (strcmp(cmd, "checkEnv") == 0)
            check_env(input, i);
        else if (cmd[0] == '\0')
            continue;
        else
            general_cmd(input);
    }

    return 0;
}
