#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>


#define MAX_NUMBERS 20
#define MAX_TARG_ARR_SIZE 20

FILE *log_file;
time_t current_time;
struct tm *time_info;
char time_string[20];
char coordinates_buffer[1024];

typedef struct
{
    float DRONE_X_LI;
    float DRONE_Y_LI;
} DIMNESIONS;

DIMNESIONS dimentions;

typedef struct
{
    float x;
    float y;

} Number;

Number number[MAX_TARG_ARR_SIZE];

// Functions

// Utility methods to spawn random container within the hoist's workspace
void initialize_numbers()
{
    srand(time(NULL));

    for (int i = 0; i < MAX_NUMBERS; i++)
    {
        number[i].x = (float)rand() / RAND_MAX * dimentions.DRONE_X_LI;
        number[i].y = (float)rand() / RAND_MAX * dimentions.DRONE_Y_LI;

        // append target coordinated to the message

        snprintf(coordinates_buffer + strlen(coordinates_buffer), sizeof(coordinates_buffer) - strlen(coordinates_buffer), "%.3f,%.3f", number[i].x, number[i].y);
        if (i < MAX_NUMBERS - 1)
        {
            snprintf(coordinates_buffer + strlen(coordinates_buffer), sizeof(coordinates_buffer) - strlen(coordinates_buffer), "|");
        }
    }

}

int main(int argc, char const *argv[])
{
    // Check for the correct number of arguments
    if (argc != 3)
    {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        return -1;
    }

    const char *ip_address = argv[1];
    int port = atoi(argv[2]);

    // logging

    // Open log file for writing
    log_file = fopen("./Logs/Numbers_log.txt", "w");
    if (log_file == NULL)
    {
        perror("Error opening the file for writing.\n");
        return -1;
    }
    // Get current time
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);

    int sock = 0;
    struct sockaddr_in serv_addr;
    char client_buffer[1024] = "TI";
    char window_buffer[1024];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form

    if (inet_pton(AF_INET, ip_address, &serv_addr.sin_addr) <= 0)
    //if (inet_pton(AF_INET, "172.20.10.10", &serv_addr.sin_addr) <= 0)
    {
        perror("\nInvalid address/ Address not supported \n");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("\nConnection Failed \n");
        exit(EXIT_FAILURE);
    }

    write(sock, &client_buffer, sizeof(client_buffer));
    fprintf(log_file, "[%s] ID sent : %s\n", time_string, client_buffer);
    fflush(log_file);

    read(sock, &client_buffer, sizeof(client_buffer));
    fprintf(log_file, "[%s] echo received : %s\n", time_string, client_buffer);
    fflush(log_file);

    read(sock, &window_buffer, sizeof(window_buffer));
    fprintf(log_file, "[%s] window buffer received : %s\n", time_string, window_buffer);
    fflush(log_file);

    write(sock, &window_buffer, sizeof(window_buffer));
    fprintf(log_file, "[%s] echo sent : %s\n", time_string, window_buffer);
    fflush(log_file);

    sscanf(window_buffer, "%f,%f", &dimentions.DRONE_X_LI, &dimentions.DRONE_Y_LI);

    // format the message
    snprintf(coordinates_buffer, sizeof(coordinates_buffer), "T[%d]", MAX_NUMBERS);

    initialize_numbers();

    write(sock, coordinates_buffer, strlen(coordinates_buffer));
    fprintf(log_file, "[%s] target coordinates : %s\n", time_string, coordinates_buffer);
    fflush(log_file);

    read(sock, &coordinates_buffer, sizeof(coordinates_buffer));
    fprintf(log_file, "[%s] echo received : %s\n", time_string, coordinates_buffer);
    fflush(log_file);

    while (TRUE)
    {

        read(sock, &coordinates_buffer, sizeof(coordinates_buffer));
        // fprintf(log_file, "[%s] notification received : %s\n", time_string,coordinates_buffer);
        // fflush(log_file);
       
        if (strcmp(coordinates_buffer, "GE") == 0)
        {
            fprintf(log_file, "[%s] Game ended\n", time_string);
            fflush(log_file);

            write(sock, &coordinates_buffer, sizeof(coordinates_buffer));
            fprintf(log_file, "[%s] echo sent : %s\n", time_string, coordinates_buffer);
            fflush(log_file);

            snprintf(coordinates_buffer, sizeof(coordinates_buffer), "T[%d]", MAX_NUMBERS);

            initialize_numbers();

            write(sock, coordinates_buffer, strlen(coordinates_buffer));
            fprintf(log_file, "[%s] target coordinates : %s\n", time_string, coordinates_buffer);
            fflush(log_file);

            read(sock, &coordinates_buffer, sizeof(coordinates_buffer));
            fprintf(log_file, "[%s] echo received : %s\n", time_string, coordinates_buffer);
            fflush(log_file);
        }
        if (strcmp(coordinates_buffer, "STOP") == 0)
        {   
            fprintf(log_file, "[%s] STOP received\n", time_string);
            fflush(log_file); 
            write (sock, "STOP", sizeof("STOP")); 
            fprintf(log_file, "[%s] STOP sent \n", time_string);
            close (sock);
            fprintf(log_file, "[%s] connection ended \n", time_string);
            fflush(log_file);
            break;
        }
    }

    close(sock);

    return 0;
}
