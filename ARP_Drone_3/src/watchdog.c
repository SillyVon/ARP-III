#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <semaphore.h>
#include <string.h>

#define MAX_PIDS 2
#define MAX_RETRIES 3  // Maximum number of retries for sending the signal

int process[MAX_PIDS];

int sendSignal(int process, FILE *file) {
    if (kill(process, SIGUSR2) == 0) {
        printf("Signal is successfully sent to %d\n", process);

        // Log that the targeted process is working
        fprintf(file, "%d process is working.\n", process);

        return 0;  // Signal sent successfully
    } else {
        perror("Send signal : ");
        return -1;  // Failed to send signal
    }
}

int main() {
    sleep(1);
    // Open the Processes PIDs log file in reading mode 
    FILE *pid_file = fopen("./Logs/pid_logs.txt", "r");
    if (pid_file == NULL) {
        perror(" WD Error opening PID file");
        exit(EXIT_FAILURE);
    }

    // Display the received PIDs
    for (int i = 0; i < MAX_PIDS; ++i) {
        if (fscanf(pid_file, " %d", &process[i]) == 2)
        {
            printf(" The process with PID : %d is registered \n", process[i]);
        }
    }

    // Boolean to check of one of the watched processes collapsed
    bool brk = false;

    FILE *file = fopen("./Logs/watchdog_log.txt", "a");
    if (file == NULL) {
        perror(" WATCHDOG Error LOG FILE:");
        return -1;
    }

    while (1) {

        for (int i = 0; i < MAX_PIDS; i++) {

            if (sendSignal(process[i], file) == 0) {
                // Signal sent successfully, continue with the next target
            } else {
                // Failed to send signal on the first attempt, retry multiple times
                fprintf(file, "Reattempting to signal process with id %d \n",process[i]);
                printf("Reattepting to signal process with id %d\n",process[i]);

                for (int retry = 0; retry < MAX_RETRIES; retry++) {
                    if (sendSignal(process[i], file) == 0) {
                        // Signal sent successfully, break out of the retry loop
                        fprintf(file, "Signal successfully sent to process %d\n", process[i]);
                        printf("Signal successfully sent to process %d\n", process[i]);
                        break;
                    }

                    // Sleep for a short duration before the next retry
                    sleep(1);
                }

                // If the process is still not responding after multiple attempts, log the failure and break out of the main loop
                fprintf(file, "%d process not responding after multiple attempts.\n",process[i]);
                printf("Process %d is not responding after multiple attempts.\n",process[i]);
                brk = true;
                break; // Break out of the loop
            }

            // Flush the file buffer to ensure that the logs are written to the file
            fflush(file);
        }

        if (brk) {
            // Perform cleanup actions
            for (int j = 0; j < MAX_PIDS; j++) {
                if (kill(process[j], SIGKILL) == 0) {
                    printf("SIGKILL Sent to %d process.\n",process[j]);
                    fprintf(file, "SIGKILL Sent to %d process.\n",process[j]);

                }
            }

            fprintf(file, "ALL PROCESSES ARE TERMINATED.\n");

            break;  // Break out of the main loop
        }

        sleep(2);
    }

    // Close all open files
    fclose(file); 
    fclose(pid_file); 

    return 0;
}