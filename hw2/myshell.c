#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define START_SIZE 10
#define BACKGROUND_EXEC "&"
#define FILE_REDIRECT ">"
#define PIPE_OUTPUT "|"

#define READ_END 0
#define WRITE_END 1

int bg_proccess_count = 0;


// TODO:
// 2. ignore signint
// 3. handle errors
// 4. handle wait/fork/exec/pipe/dup2/sig assign errors

void sigchld_handler( int        signum,
                      siginfo_t* info,
                      void*      ptr)
{
    unsigned long chld_pid = (unsigned long) info->si_pid;
    printf("The child pid is: %li\n", chld_pid);
    waitpid(chld_pid, NULL, WNOHANG);
    if (errno != ECHILD && errno != 0)
    {
        perror("error in signal handler");
        exit(1);
    }
}

int reset_sigint_action()
{
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_handler = SIG_DFL;
    if( 0 != sigaction(SIGINT, &new_action, NULL) )
    {
        perror("Error assiging defaul handler on SIGINT");
        return 1;
    }
    return 0;
}

int prepare(void)
{
    // Structure to pass to the registration syscall
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_handler = SIG_IGN;
    if( 0 != sigaction(SIGINT, &new_action, NULL) )
    {
        perror("Error assiging ignroe handler on SIGINT");
        return 1;
    }
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_sigaction = sigchld_handler;
    new_action.sa_flags = SA_SIGINFO;
    // Register the handler
    // if( 0 != sigaction(SIGCHLD, &new_action, NULL) )
    // {
    //     perror("Error assinging SIGCHLD new wait handler");
    //     return 1;
    // }
    return 0;
}

int finalize(void)
{
    return 0;
}

int hunt_zombies()
{
    while ((bg_proccess_count > 0) && ( waitpid(-1, NULL, WNOHANG) > 0))
    {
        bg_proccess_count--;
    }
    return 0;    
}

int exec_front(char **arglist)
{
    int pid = fork();
    if (pid == 0)
    {
        reset_sigint_action();
        execvp(arglist[0], arglist);
        perror("Error executing foreground proccess");
        return 0; // signaling its a child proccess
    }
    else
    {
        waitpid(pid, NULL, 0);
        return 1;
    }
}

int exec_back(char **arglist)
{
    int pid = fork();
    if (pid == 0)
    {
        execvp(arglist[0], arglist);
        perror("Error executing background proccess");
        return 0; // signaling its a child proccess
    }
    else
    {
        bg_proccess_count++;
        return 1;
    }
}

int redirect_exec(char **arglist, char *outfile)
{
    int res = 0;
    int fout_desc = open(outfile, O_WRONLY | O_TRUNC | O_CREAT, 0666);
    int store_stdout = dup(STDOUT_FILENO);
    dup2(fout_desc, STDOUT_FILENO);
    res = exec_front(arglist);
    dup2(store_stdout, STDOUT_FILENO);
    return res;
}

int piped_exec(char **cmd1, char **cmd2)
{
    int fd[2], c1_pid, c2_pid;
    if (pipe(fd))
    {
        perror("pipe fail");
        exit(1);
    }
    // first child write to pipe
    c1_pid = fork();
    if (c1_pid == 0)
    {
        reset_sigint_action();
        dup2(fd[WRITE_END], STDOUT_FILENO);
        close(fd[WRITE_END]);
        close(fd[READ_END]);
        execvp(cmd1[0], cmd1);
        perror("Faild to exec first command from pipe");
        return 0;
    }
    // second child (read from pipe)
    c2_pid = fork();
    if (c2_pid == 0)
    {
        reset_sigint_action();
        dup2(fd[READ_END], STDIN_FILENO);
        close(fd[WRITE_END]);
        close(fd[READ_END]);
        printf("%s %s %s", cmd2[0], cmd2[1], cmd2[2]);
        execvp(cmd2[0], cmd2);
        perror("Faild to exec second command from pipe");
        return 0;
    }
    close(fd[WRITE_END]);
    close(fd[READ_END]);
    waitpid(c1_pid, NULL, 0);
    waitpid(c2_pid, NULL, 0);
    return 1;
}

int find_word(int count, char **arglist, char *word)
{
    for (int i = 0; i < count; i++)
    {
        if (strcmp(arglist[i], word) == 0)
        {
            return i;
        }
    }
    return -1;
}

int process_arglist(int count, char **arglist)
{
    int index = -1;
    hunt_zombies();
    if ((index = find_word(count, arglist, PIPE_OUTPUT)) != -1)
    {
        char **cmd1, **cmd2;
        arglist[index] = NULL;
        cmd1 = arglist;
        cmd2 = &(arglist[index + 1]);
        return piped_exec(cmd1, cmd2);
    }
    else if ((index = find_word(count, arglist, FILE_REDIRECT)) != -1)
    {
        arglist[index] = NULL;
        return redirect_exec(arglist, arglist[index + 1]);
    }
    else if (strcmp(arglist[count - 1], BACKGROUND_EXEC) == 0)
    {
        arglist[count - 1] = NULL;
        return exec_back(arglist);
    } else {
        return exec_front(arglist);
    }
}