#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sys/select.h"
#include <pthread.h>
#include <signal.h>

#define MAX_OBSTACLES 20
#define M 1.0
#define K 1.0
#define T 0.3

#define RHO 7.0    // Distance of influence  
#define ETA_MIN 0.9 // Minimum repulsion force
#define ETA_MAX 1.0 // Maximum repulsion force  

FILE *log_file;
time_t current_time;
struct tm *time_info;
char time_string[20];

// global variables


int obstacle_coordinates_fd;
char *obstacle_coordinates_fifo = "/tmp/obstacle_motion";


// Drone coordinates
typedef struct
{
    float ee_y; // the current position
    float ee_x; // the current position

} Coordinates;

Coordinates coordinates;

typedef struct
{
    int on_x;
    int on_y;
} Forces;

static Forces forces;

typedef struct
{
    float x;
    float y; 
    int value;       

} Obstacle;

Obstacle obstacle[MAX_OBSTACLES];

// Functions

// resetting function
void reset_program()
{

    float xi_minus_2; // the 2.previous postion on x
    float xi_minus_1; // the previous postion on x
    float yi_minus_2; // the 2.previous postion on Y
    float yi_minus_1; // the  previous positon on Y

    // Reset all relevant variables to their initial values
    xi_minus_2 = 0;
    xi_minus_1 = 0;
    yi_minus_2 = 0;
    yi_minus_1 = 0;
    coordinates.ee_x = 0;
    coordinates.ee_y = 0;
    forces.on_x = 0;
    forces.on_y = 0;
}

void motion_method(Forces forces)
{

    static float xi_minus_2; // the 2.previous postion on x
    static float xi_minus_1; // the previous postion on x
    static float yi_minus_2; // the 2.previous postion on Y
    static float yi_minus_1; // the  previous positon on Y
    // Euler's method for position calculation
    coordinates.ee_x = ((2 * M + K * T) * xi_minus_1 - M * xi_minus_2 + T * T * forces.on_x) / (K * T + M);
    coordinates.ee_y = ((2 * M + K * T) * yi_minus_1 - M * yi_minus_2 + T * T * forces.on_y) / (K * T + M);

    xi_minus_2 = xi_minus_1;
    xi_minus_1 = coordinates.ee_x;
    yi_minus_2 = yi_minus_1;
    yi_minus_1 = coordinates.ee_y;
}

// function to recieve the direction from the user
int direction_tracker(char direction)
{

    switch (direction)
    {

    case 'E':
    case 'e':
        return 1; // move North
        break;
    case 'C':
    case 'c':
        return 2; // move South
        break;
    case 'S':
    case 's':
        return 3; // move West
        break;
    case 'F':
    case 'f':
        return 4; // move East
        break;
    case 'R':
    case 'r':
        return 5; // move Northeast
        break;
    case 'W':
    case 'w':
        return 6; // move Northwest
        break;
    case 'X':
    case 'x':
        return 7; // move Southwest
        break;
    case 'V':
    case 'v':
        return 8; // move Southeast
        break;
    case 'Q':
    case 'q':
        // pressing 'Q' terminates the program
        endwin();
        exit(EXIT_SUCCESS);

    case 'D':
    case 'd':
        return 0; // pressing 'D' stops the drone
        break;
    case 'Z':
    case 'z':
        reset_program();
        break;

    default:
        return -1; // invalid input
    }
}

// Function to check for collisions between the drone and obstacles
int check_collision(Coordinates coordinates)
{   

    for (int i = 0; i < MAX_OBSTACLES; i++)
    {
        // Check if the drone is close enough to the obstacle
        if (fabs(coordinates.ee_x - obstacle[i].x) < 2 && fabs(coordinates.ee_y - obstacle[i].y) < 2)
        {
            printf("Collision detected with obstacle at (%.3f, %.3f)\n", obstacle[i].x, obstacle[i].y);
            return 1; // Collision detected
        }
    }
    return 0; // No collision
}

void repulsive_forces()
{   
    while (true){
        
        for (int i = 0; i < MAX_OBSTACLES; i++){
            printf("Obstacle in repulsive forces %d: (%.3f, %.3f)\n",i, obstacle[i].x, obstacle[i].y);
            }
            sleep(1);
        
        
        for (int i = 0; i < MAX_OBSTACLES; i++){

            if (fabs(coordinates.ee_x - obstacle[i].x) < 1 && fabs(coordinates.ee_y - obstacle[i].y) < 1)
            {   
                printf("Collision detected with obstacle at (%.3f, %.3f)\n", obstacle[i].x, obstacle[i].y);
                // Collision detected, stop the drone
                forces.on_x = 0;
                forces.on_y = 0;
                // return forces;
            }
            else
            {
                // Calculate and apply repulsion forces

                for (int i = 0; i < MAX_OBSTACLES; i++)
                {
                    float dx = coordinates.ee_x - (int)obstacle[i].x;
                    float dy = coordinates.ee_y - (int)obstacle[i].y;
                    float distance = sqrt(dx * dx + dy * dy);

                    if (distance < RHO)
                    {
                        // Calculate repulsion force magnitude
                        float eta = (ETA_MAX - ETA_MIN) * (1.0 - distance / RHO) + ETA_MIN;

                        // Normalize the repulsion vector
                        float repulsion_x = dx / distance;
                        float repulsion_y = dy / distance;

                        // Apply repulsion force
                        forces.on_x += eta * repulsion_x;
                        forces.on_y += eta * repulsion_y;
                    }
                }
            }   // return forces;
        }
    }
    
}

void *recieve_obstacles(void *arg)
{   
    obstacle_coordinates_fd = open(obstacle_coordinates_fifo, O_RDONLY);
    
    while (TRUE)
    {
        // obstacle_coordinates_fd has data
        for (int i = 0; i < MAX_OBSTACLES; i++)
        {  
        ssize_t bytesRead_obstacles = read(obstacle_coordinates_fd, &obstacle[i], sizeof(obstacle[i]));
        if (bytesRead_obstacles > 0)
        {   
            // If data is read from the pipe, print it
            //printf("Obstacle %d: (%.3f, %.3f)\n",i, obstacle[i].x, obstacle[i].y);   
        }
        }
        
        repulsive_forces();
        //motion_method(forces);
    }
    close(obstacle_coordinates_fd);
}

void sig_handler(int signo)
{
    usleep(100);
}

int main(int argc, char const *argv[])
{
    // creating signal handler
    signal(SIGUSR2, sig_handler);

    //log file for pids
    FILE *motion_pid_log = fopen("./Logs/pid_logs.txt", "a");
    if (motion_pid_log == NULL)
    {
        perror("Error opening the file for writing.\n");
        return -1;
    }
    // get pid of the main process
    pid_t motion_pid = getpid();
    fprintf(motion_pid_log, "%d ", motion_pid);
    fclose(motion_pid_log);

    int motion_direction = 0; // 0: No motion, 1: North, 2: South, 3: West, 4: East, 5: Northeast, 6: Northwest, 7: Southwest, 8: Southeast

    // forces increments
    const int force_increment = 1;  // incrementing by 1 N
    const int force_decrement = -1; // decrementing by 1 N

    // file descriptor for pipes
    int input_fd;
    int coordinates_fd;

    // FIFOs
    char *input_fifo = "/tmp/input_fifo";
    char *coordinates_fifo = "/tmp/coordinates_fifo";
    mkfifo(obstacle_coordinates_fifo, 0666);

    // Create the FIFOs
    mkfifo(input_fifo, 0666);
    mkfifo(coordinates_fifo, 0666);

    // Create a thread to recieve obstacles
    pthread_t repulsive_forces_thread;
    pthread_create(&repulsive_forces_thread, NULL, recieve_obstacles, NULL);

    // logging

    // Open log file for writing
    log_file = fopen("./Logs/Motion_Dynamics_log.txt", "w");
    if (log_file == NULL)
    {
        perror("Error opening the file for writing.\n");
        return -1;
    }
    // Get current time
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);

    while (TRUE)
    {
        // printf("STill in the while loop\n");
        input_fd = open(input_fifo, O_RDONLY);
        char input_received;
        ssize_t bytesRead = read(input_fd, &input_received, sizeof(input_received));
        close(input_fd);

        // determine the direction
        motion_direction = direction_tracker(input_received);

        // Switch case for the 8 directions:
        switch (motion_direction)
        {
        case 1: // North
            forces.on_y += force_decrement;
            break;
        case 2: // South
            forces.on_y += force_increment;
            break;
        case 3: // West
            forces.on_x += force_decrement;
            break;
        case 4: // East
            forces.on_x += force_increment;
            break;
        case 5: // Northeast
            forces.on_x += force_increment;
            forces.on_y += force_decrement;
            break;
        case 6: // Northwest
            forces.on_x += force_decrement;
            forces.on_y += force_decrement;
            break;
        case 7: // Southwest
            forces.on_x += force_decrement;
            forces.on_y += force_increment;
            break;
        case 8: // Southeast
            forces.on_x += force_increment;
            forces.on_y += force_increment;
            break;
        case 0:
            forces.on_x = 0;
            forces.on_y = 0;
            break;

        case -1:
            // no input, use old forces
            break;
        default:
            // handle invalid motion direction
            break;
        }

        motion_method(forces);
        coordinates_fd = open(coordinates_fifo, O_WRONLY);
        write(coordinates_fd, &coordinates, sizeof(coordinates));
        close(coordinates_fd);
        
        // logging
        fprintf(log_file, "[%s]The process is updated.\n", time_string);
        fflush(log_file);
        fprintf(log_file, "[%s]Forces updated on X and Y are : (%d, %d)\n", time_string, forces.on_x, forces.on_y);
        fflush(log_file);
        fprintf(log_file, "[%s] Coordinates updated are : (%f, %f)\n", time_string, coordinates.ee_x, coordinates.ee_y);
        fflush(log_file);
    }
    return 0;
}
