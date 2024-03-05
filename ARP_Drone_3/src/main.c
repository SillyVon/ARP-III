#include "./../include/main.h"

// functions used with sockets and threads
void *drone_movement(void *arg)
{

    // Infinite loop
    while (TRUE)
    {

        input_fd = open(input_fifo, O_WRONLY);
        int ch = getch(); // non-blocking input
        if (ch != ERR)
        {
            if (ch == 'Q' || ch == 'q')
            {

                write(input_fd, "Q", sizeof("Q"));
                // If Q is pressed, exit the program
                stop_connection_flag = TRUE;
                break;
            }

            char direction_input;
            direction_input = (char)ch;
            write(input_fd, &direction_input, sizeof(direction_input));
        }
        else
        {
            write(input_fd, &empty, sizeof(empty));
        }
        close(input_fd);

        // i recieve the ee_X ee_y from motion_dynmaics process through pipes
        coordinates_fd = open(coordinates_fifo, O_RDONLY);
        ssize_t bytesRead;
        // do keeps reading from the pipe until no more data is available.
        do
        {
            bytesRead = read(coordinates_fd, &coordinates, sizeof(coordinates));
            if (bytesRead == sizeof(coordinates))
            {
                update_console_ui(&coordinates.ee_x, &coordinates.ee_y);
            }

        } while (bytesRead == sizeof(coordinates));
        close(coordinates_fd);
        usleep(100000);
    }
    return NULL;
}

//passing functions
void parse_coordinates(const char *message)
{
    // parse the identifier
    char identifier;
    sscanf(message, "%c", &identifier);

    // parse the number of targets

    sscanf(message, "%*[^[][%d]", &num_targets);

    // parse the target coordinates
    const char *coord_str = strchr(message, '|') + 1; // fist occurence of '|'
    for (int i = 0; i < num_targets; i++)
    {
        sscanf(coord_str, "%f,%f", &number[i].x, &number[i].y);
        // Initialize value to 1 for all numbers
        number[i].value = i + 1;
        coord_str = strchr(coord_str, '|');
        if (coord_str == NULL)

            break;
        ++coord_str;
    }
}

void parse_coordinates_obstacles(const char *message)
{
    // parse the identifier
    char identifier;
    sscanf(message, "%c", &identifier);

    // Find the position of the first '|' which marks the beginning of coordinates
    const char *coord_start = strchr(message, '|') + 1;
    if (coord_start == NULL)
    {
        // Error handling: no coordinates found
        return;
    }
    // Parse the number of obstacles
    sscanf(message, "%*[^[][%d]", &num_obstacles);

    // parse the target coordinates
    const char *coord_str = coord_start + 1;
    for (int i = 0; i < num_obstacles; i++)
    {
        sscanf(coord_str, "%f,%f", &obstacle[i].x, &obstacle[i].y);
        //fprintf(log_file, "[%s] obstacle %d info: x: %.3f y: %.3f\n", time_string, i, obstacle[i].x, obstacle[i].y);
        //fflush(log_file);
        // Initialize value to 1 for all numbers
        obstacle[i].value = i + 1;
        coord_str = strchr(coord_str, '|');
        if (coord_str == NULL)
            break;
        ++coord_str;
    }

    update_obstacle_motion();
}

void *handle_client(void *arg)
{

    int client_socket = *((int *)arg);

    // sending the blackboard dimensions to the client
    dimentions.DRONE_X_LI = 100;
    dimentions.DRONE_Y_LI = 20;

    // creating the string buffer for window
    char window_buffer[1024];
    char client_buffer[1024];
    sprintf(window_buffer, "%.3f,%.3f", dimentions.DRONE_X_LI, dimentions.DRONE_Y_LI);

    // window_buffer contains the string "100.000,20.000"

    // recieving the process id
    if (read(client_socket, &client_buffer, sizeof(client_buffer)) < 0)
    {
        perror("Identification message");
        exit(EXIT_FAILURE);
    }

    fprintf(log_file, "[%s] ID received : %s\n", time_string, client_buffer);
    fflush(log_file);

    // Echoing back to the client
    if (write(client_socket, &client_buffer, sizeof(client_buffer)) < 0)
    {
        perror("failed echo messaging ");
        exit(EXIT_FAILURE);
    }
    fprintf(log_file, "[%s] echo sent : %s\n ", time_string, client_buffer);
    fflush(log_file);

    // sending the blackboard dimensions to the client
    if (write(client_socket, &window_buffer, sizeof(window_buffer)) < 0)
    {
        perror("sending blackboard dimensions to the client");
        exit(EXIT_FAILURE);
    }
    fprintf(log_file, "[%s]The window buffer sent is : %s\n", time_string, window_buffer);
    fflush(log_file);
    // recieving the echo message
    if (read(client_socket, &window_buffer, sizeof(window_buffer)) < 0)
    {
        perror("failed Echo messaging");
        exit(EXIT_FAILURE);
    }
    fprintf(log_file, "[%s] echo message received : %s\n", time_string, window_buffer);
    fflush(log_file);

    if (strncmp(client_buffer, "TI", 2) == 0)
    {

        if (read(client_socket, &client_buffer, sizeof(client_buffer)) < 0)
        {
            perror("receiving the target coordinates");
            exit(EXIT_FAILURE);
        }
        fprintf(log_file, "[%s] target coordinates received : %s\n", time_string, client_buffer);
        fflush(log_file);

        if (write(client_socket, &client_buffer, sizeof(client_buffer)) < 0)
        {
            perror("failed echo messaging ");
            exit(EXIT_FAILURE);
        }
        fprintf(log_file, "[%s] echo sent : %s\n ", time_string, client_buffer);
        fflush(log_file);

        parse_coordinates(client_buffer);

        while (TRUE)
        {
            if (stop_connection_flag)
            {
                write(client_socket, "STOP", sizeof("STOP"));
                fprintf(log_file, "[%s]targets STOP\n", time_string);
                fflush(log_file);
                read(client_socket, &client_buffer, sizeof(client_buffer));
                fprintf(log_file, "[%s]echo received:%s\n", time_string, client_buffer);
                fflush(log_file);
                endwin();
                exit(EXIT_SUCCESS);
            }
            if (all_numbers_zero())
            {

                strcpy(client_buffer, "GE");
                if (write(client_socket, &client_buffer, sizeof(client_buffer)) < 0)
                {
                    perror("failed echo messaging ");
                    exit(EXIT_FAILURE);
                }
                fprintf(log_file, "[%s] notification sent: %s\n ", time_string, client_buffer);
                fflush(log_file);

                if (read(client_socket, &client_buffer, sizeof(client_buffer)) < 0)
                {
                    perror("failed echo messaging ");
                    exit(EXIT_FAILURE);
                }
                fprintf(log_file, "[%s] echo received : %s\n ", time_string, client_buffer);
                fflush(log_file);

                if (read(client_socket, &client_buffer, sizeof(client_buffer)) < 0)
                {
                    perror("receiving the target coordinates");
                    exit(EXIT_FAILURE);
                }
                fprintf(log_file, "[%s] target coordinates received : %s\n ", time_string, client_buffer);
                fflush(log_file);

                if (write(client_socket, &client_buffer, sizeof(client_buffer)) < 0)
                {
                    perror("failed echo messaging ");
                    exit(EXIT_FAILURE);
                }
                fprintf(log_file, "[%s] echo sent : %s\n ", time_string, client_buffer);
                fflush(log_file);

                parse_coordinates(client_buffer);
            }
            strcpy(client_buffer, "GE");
        }
    }
    else if (strncmp(client_buffer, "OI", 2) == 0)
    {
        while (TRUE)
        {
            if (stop_connection_flag)
            {   
                /*
                write(client_socket, "STOP", sizeof("STOP"));
                fprintf(log_file, "[%s]obstacles STOP\n", time_string);
                fflush(log_file);
                read(client_socket, &client_buffer, sizeof(client_buffer));
                fprintf(log_file, "[%s]echo received:%s\n", time_string, client_buffer);
                fflush(log_file);
                */
                endwin();
                exit(EXIT_SUCCESS);
            }
            
            // recieving the obstacle coordinates
            if (read(client_socket, &client_buffer, sizeof(client_buffer)) < 0)
            {
                perror("receiving the obstacle coordinates");
                exit(EXIT_FAILURE);
            }
            fprintf(log_file, "[%s] obstacle coordinates received : %s\n", time_string, client_buffer);
            fflush(log_file);
            // Echoing back to the client
            if (write(client_socket, &client_buffer, sizeof(client_buffer)) < 0)
            {
                perror("failed echo messaging ");
                exit(EXIT_FAILURE);
            }
            fprintf(log_file, "[%s] echo sent : %s\n ", time_string, client_buffer);
            fflush(log_file);
            // parse the coordinates
            parse_coordinates_obstacles(client_buffer);
        }
    }
    // close the client socket and exit the thread
    close(client_socket);
    pthread_exit(NULL);
}

// signal handler
void sig_handler(int signo)
{
    usleep(100);
}

int main(int argc, char const *argv[])
{
    // creating signal handler
    signal(SIGUSR2, sig_handler);

    //log file for pids
    FILE *main_pid_log = fopen("./Logs/pid_logs.txt", "a");
    if (main_pid_log == NULL)
    {
        perror("Error opening the file for writing.\n");
        return -1;
    }
    // get pid of the main process
    pid_t main_pid = getpid();
    fprintf(main_pid_log, "%d ", main_pid);
    fclose(main_pid_log);

    // Open log file for writing
    log_file = fopen("./Logs/main_log.txt", "w");
    if (log_file == NULL)
    {
        perror("Error opening the file for writing.\n");
        return -1;
    }
    //  Get current time
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);

    // ceating fifo files
    mkfifo(input_fifo, 0666);
    mkfifo(coordinates_fifo, 0666);

    // file descriptor for the sockets
    int server_fd, new_socket, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    pthread_t threads[MAX_CLIENTS];
    pthread_t movement_thread;
    pthread_t obstacle_thread;
     int i = 0;

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize User Interface
    init_console_ui();

    nodelay(stdscr, TRUE); // getch() will be non-blocking

    pthread_create(&movement_thread, NULL, drone_movement, NULL);
    
    // creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    };

    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // accept and handle the client connections
    while (i < MAX_CLIENTS)
    {
        //
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            // exit(EXIT_FAILURE);
            continue;
        }

        // create a new thread to handle the client connection

        if (pthread_create(&threads[i], NULL, handle_client, (void *)&new_socket) != 0)
        {
            perror("pthread_create");
            close(new_socket);
            continue;
        }
        // printf("Client connected. Thread created to handle the connection.\n");
        i++;
    }

    // wait for all threads to finish
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        pthread_join(threads[i], NULL);
    }
    // close the server socket
    close(server_fd);
    
    // wait for the movement thread to finish
    pthread_join(movement_thread, NULL);

    // Terminate
    endwin();
    return 0;
}