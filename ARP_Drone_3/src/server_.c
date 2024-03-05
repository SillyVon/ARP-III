#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_PIDS 3

// logging

// initializing log file
FILE *log_file;
time_t current_time;
struct tm *time_info;
char time_string[20];

int main(int argc, char *argv[])
{

    // pids of the children processes
    pid_t child_pid;
    pid_t child_main;
    pid_t child_motion;
    pid_t child_watchdog;

    // Array to store child process IDs
    pid_t child_pids[MAX_PIDS] = {child_main, child_motion, child_watchdog};

    // termination status
    int res;

    // logging

    // Open log file for writing
    log_file = fopen("./Logs/server_log.txt", "w");
    if (log_file == NULL)
    {
        perror("Error opening the file for writing.\n");
        return -1;
    }
    // Get current time
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);

    // Make main child process
    fprintf(log_file, "[%s]Forking the main process\n", time_string);
    fflush(log_file);
    child_pids[0] = fork();
    if (child_pids[0] < 0)
    {
        perror("error while forking...");
        return -1;
    }

    if (child_pids[0] == 0)
    {
        char *arg_list_main[] = {"konsole", "-e", "./bin/main", NULL};
        execvp("konsole", arg_list_main);
        return 0;
    }

    // Make motion child process
    fprintf(log_file, "[%s]Forking the Motion_Dynamics process\n", time_string);
    fflush(log_file);
    child_pids[1] = fork();
    if (child_pids[1] < 0)
    {
        perror("error while forking...");
        return -1;
    }
    if (child_pids[1] == 0)
    {
        char *arg_list_motion[] = {"terminator", "--command", "./bin/motion", NULL};
        execvp("terminator", arg_list_motion);
        return 0;
    }

    // Make watchdog child process
    fprintf(log_file, "[%s]Forking the Watchdog process\n", time_string);
    fflush(log_file);
    child_pids[2] = fork();
    if (child_pids[2] < 0)
    {
        perror("error while forking ...");
        return -1;
    }
    if (child_pids[2] == 0)
    {
        char *arg_list_watchdog[] = {"terminator", "--command", "./bin/watchdog", NULL};
        execvp("terminator", arg_list_watchdog);
        return 0;
    }

    fprintf(log_file, "[%s]Process Main forked successfully,Process ID: %d\n", time_string, child_pids[0]);
    fflush(log_file);
    fprintf(log_file, "[%s]Process Motion_dynamics forked successfully,Process ID: %d\n", time_string, child_pids[1]);
    fflush(log_file);
    fprintf(log_file, "[%s]Process Watchdog forked successfully,Process ID: %d\n", time_string, child_pids[2]);
    fflush(log_file);

    // wait for children to finish

    for (int i = 0; i < MAX_PIDS; i++)
    {
        child_pid = wait(&res);
        printf("Child process with PID %d exited with status %d\n", child_pid, res);
    }

    // Close log file
    fclose(log_file);

    return 0;
}