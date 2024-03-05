#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <arpa/inet.h>
#include <string.h>

#define TRUE 1
#define MAX_OBSTACLES 20
#define MAX_OBST_ARR_SIZE 20
// 127.0.0.1

char coordinates_buffer[1024];

FILE *log_file;
time_t current_time_log;
struct tm *time_info;
char time_string[20];

typedef struct
{
    float x;
    float y;
} Obstacle;

Obstacle obstacle[MAX_OBST_ARR_SIZE];

typedef struct
{
    float DRONE_X_LI;
    float DRONE_Y_LI;
} DIMNESIONS;

DIMNESIONS dimentions;

void spawn_random_obstacle()
{
    srand(time(NULL));

    for (int i = 0; i < MAX_OBSTACLES; i++)
    {
        obstacle[i].x = (float)rand() / RAND_MAX * dimentions.DRONE_X_LI;
        obstacle[i].y = (float)rand() / RAND_MAX * dimentions.DRONE_Y_LI;

        // append target coordinated to the message
        snprintf(coordinates_buffer + strlen(coordinates_buffer), sizeof(coordinates_buffer) - strlen(coordinates_buffer), "%.3f,%.3f", obstacle[i].x, obstacle[i].y);
        if (i < MAX_OBSTACLES - 1)
        {
            snprintf(coordinates_buffer + strlen(coordinates_buffer), sizeof(coordinates_buffer) - strlen(coordinates_buffer), "|");
        }
    }
}

int main(int argc, char const *argv[])
{

    // check if the number of arguments is correct
    if (argc != 3)
    {
        printf("Usage: %s <port>\n", argv[0]);
        return -1;
    }

    const char *ip_address = argv[1];
    int port = atoi(argv[2]);

    // logging

    // Open log file for writing
    log_file = fopen("./Logs/Obstacles_log.txt", "w");
    if (log_file == NULL)
    {
        perror("Error opening the file for writing.\n");
        return -1;
    }
    // Get current time
    time(&current_time_log);
    time_info = localtime(&current_time_log);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);

    clock_t start_time, current_time;
    double elapsed_time;

    // Set the initial start time
    start_time = clock();

    int sock = 0;
    struct sockaddr_in serv_addr;
    char client_buffer[1024] = "OI";
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
    // if(inet_pton(AF_INET, "172.20.10.10", &serv_addr.sin_addr) <= 0)
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

    // Extract the drone dimentions from the window buffer
    sscanf(window_buffer, "%f,%f", &dimentions.DRONE_X_LI, &dimentions.DRONE_Y_LI);

    // format the message
    snprintf(coordinates_buffer, sizeof(coordinates_buffer), "O[%d]", MAX_OBSTACLES);

    spawn_random_obstacle();

    write(sock, coordinates_buffer, strlen(coordinates_buffer));
    fprintf(log_file, "[%s] target coordinates : %s\n", time_string, coordinates_buffer);
    fflush(log_file);

    read(sock, &coordinates_buffer, sizeof(coordinates_buffer));
    fprintf(log_file, "[%s] echo received : %s\n", time_string, coordinates_buffer);
    fflush(log_file);

    while (TRUE)
    {

        current_time = clock();
        elapsed_time = ((double)(current_time - start_time)) / CLOCKS_PER_SEC;

        if (elapsed_time > 10.0)
        {
            snprintf(coordinates_buffer, sizeof(coordinates_buffer), "O[%d]", MAX_OBSTACLES);

            spawn_random_obstacle();

            write(sock, coordinates_buffer, strlen(coordinates_buffer));
            fprintf(log_file, "[%s] target coordinates : %s\n", time_string, coordinates_buffer);
            fflush(log_file);

            read(sock, &coordinates_buffer, sizeof(coordinates_buffer));
            fprintf(log_file, "[%s] echo received : %s\n", time_string, coordinates_buffer);
            fflush(log_file);

            start_time = clock(); // Reset the start time for the next obstacle generation
        }
    }
    return 0;
}