#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define START_SIZE 10
#define BACKGROUNG_EXEC "&"
#define FILE_REDIRECT ">"
#define PIPE_OUTPUT "|"

#define READ_END 0
#define WRITE_END 1

int bg_proccess_count = 0;


// TODO:
// 1. hunt zombies
// 2. ignore signint
// 3. handle errors

int prepare(void)
{
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
    int exit_code = -1;
    if (pid == 0)
    {
        execvp(arglist[0], arglist);
        perror("Error executing foreground proccess\n");
        return 0; // signaling its a child proccess
    }
    else
    {
        waitpid(pid, &exit_code, 0);
        return 1;
    }
}

int exec_back(char **arglist)
{
    int pid = fork();
    if (pid == 0)
    {
        execvp(arglist[0], arglist);
        perror("Error executing background proccess\n");
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
    int fout_desc = open(outfile, O_WRONLY | O_CREAT, 0666);
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
        perror("pipe fail\n");
        exit(1);
    }
    // first child write to pipe
    c1_pid = fork();
    if (c1_pid == 0)
    {
        dup2(fd[WRITE_END], STDOUT_FILENO);
        close(fd[WRITE_END]);
        close(fd[READ_END]);
        execvp(cmd1[0], cmd1);
        perror("Faild to exec first command from pipe\n");
        return 0;
    }
    // second child (read from pipe)
    c2_pid = fork();
    if (c2_pid == 0)
    {
        dup2(fd[READ_END], STDIN_FILENO);
        close(fd[WRITE_END]);
        close(fd[READ_END]);
        printf("%s %s %s", cmd2[0], cmd2[1], cmd2[2]);
        execvp(cmd2[0], cmd2);
        perror("Faild to exec second command from pipe\n");
        return 0;
    }
    int exit_code = -1;
    close(fd[WRITE_END]);
    close(fd[READ_END]);
    waitpid(c1_pid, &exit_code, 0);
    waitpid(c2_pid, &exit_code, 0);
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
    else if (strcmp(arglist[count - 1], BACKGROUNG_EXEC) == 0)
    {
        arglist[count - 1] = NULL;
        return exec_back(arglist);
    } else {
        return exec_front(arglist);
    }
}