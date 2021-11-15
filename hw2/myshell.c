#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define START_SIZE 10
#define BACKGROUNG_EXEC "&"
#define PIPE_OUTPUT "|"

int *bg_pids;
int bg_pids_size = START_SIZE;
int bg_pids_count = 0;

int prepare(void)
{
    bg_pids = (int *)malloc(sizeof(*bg_pids) * START_SIZE);
    if (bg_pids == NULL)
    {
        printf("error allocating bg_pids array\n");
        return 1;
    }
    return 0;
}

int finalize(void)
{
    for (int i = 0; i < bg_pids_count; i++)
    {
        int pid = bg_pids[i];
        int exit_code = -1;
        waitpid(pid, &exit_code, 0);
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
    int exit_code = -1;
    if (pid == 0)
    {
        execvp(arglist[0], arglist);
        return 0; // signaling its a child proccess
    }
    else
    {
        if (bg_pids_count == bg_pids_size)
        {
            bg_pids_size *= 2;
            bg_pids = (int *)realloc(bg_pids, bg_pids_size);
            if (bg_pids == NULL)
            {
                printf("error reallocating bg_pids array\n");
                return 1;
            }
            bg_pids[bg_pids_count] = pid;
            bg_pids_count++;
        }
        return 1;
    }
}

int find_word(int count, char** arglist, char* word)
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
    if (index = find_word(count, arglist, PIPE_OUTPUT) != -1)
    {
        arglist[index] = NULL;
        index++;
    }
    if (strcmp(arglist[count-1], BACKGROUNG_EXEC) == 0)
    {
        arglist[count - 1] = NULL;
        return exec_back(arglist);
    }
    return exec_front(arglist);
}