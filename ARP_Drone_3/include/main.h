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
#include <sys/select.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define PORT 3000
#define MAX_CLIENTS 2 
#define MAX_OBSTACLES 20
#define MAX_NUMBERS 20
#define MAX_TARG_ARR_SIZE 20

char *input_fifo = "/tmp/input_fifo";
char *coordinates_fifo = "/tmp/coordinates_fifo";

// Global variables

int num_targets;
int num_obstacles;
int delete_count;

// connection flag
bool stop_connection_flag = FALSE;

char empty = ' ';

// file descriptor for pipes
int input_fd;
int coordinates_fd;

// Declration

FILE *log_file;
time_t current_time;
struct tm *time_info;
char time_string[20];

// limits for the field dimenstions.
int DRONE_X_LIM = 100;
int DRONE_Y_LIM = 20;

// Drone coordinates
float ee_y; // the current position
float ee_x; // the current position

typedef struct
{
    float x;
    float y;
    int value;

} Obstacle;

Obstacle obstacle[MAX_OBSTACLES];

typedef struct
{
    float x;
    float y;
    int value;

} Number;

Number number[MAX_TARG_ARR_SIZE];

// Number erased_target;

typedef struct
{
    chtype ls, rs, ts, bs,
        tl, tr, bl, br;
} DRONE_BORDER;

typedef struct
{
    int startx, starty;
    int height, width;
    DRONE_BORDER border;
} DRONE;
// Drone structure variable
DRONE drone;

typedef struct
{
    int on_x;
    int on_y;
} Forces;
Forces forces;

typedef struct
{
    float DRONE_X_LI;
    float DRONE_Y_LI;
} DIMNESIONS;

DIMNESIONS dimentions;

// Drone coordinates
typedef struct
{
    float ee_y; // the current position
    float ee_x; // the current position
} Coordinates;
static Coordinates coordinates;

// size of the buttons.
int BTN_SIZE = 7;

// Pointers to button windows
WINDOW *stp_button, *rst_button, *quit_button;

// Functions

// resetting function
void reset_program()
{

    coordinates.ee_x = 0;
    coordinates.ee_y = 0;
}

// Initialize the drone structure and parameters :

void make_drone_field()
{

    // limits for the field dimenstions.
    drone.height = DRONE_Y_LIM;
    drone.width = DRONE_X_LIM;

    // postion of the borders
    drone.starty = 6;
    drone.startx = (COLS - drone.width) / 2;

    drone.border.tl = ACS_ULCORNER; // topleft
    drone.border.tr = ACS_URCORNER; // topright
    drone.border.bl = ACS_LLCORNER; // bottomleft
    drone.border.br = ACS_LRCORNER; // bottomright
}

// drawing the drone and the structure.

void draw_drone_field()
{
    int x, y, w, h;

    x = drone.startx;
    y = drone.starty;
    w = drone.width;
    h = drone.height;

    // Draw the entire drone structure

    mvaddch(y - 1, x - 1, drone.border.tl); //
    mvaddch(y - 1, x + w, drone.border.tr);
    mvaddch(y + h - 1, x - 1, drone.border.bl);
    mvaddch(y + h - 1, x + w, drone.border.br);

    refresh();
}
// Place a title  with the drones coordinates on top

void draw_drone_msg(float x, float y)
{

    // clearing the line for the title to be printed
    for (int j = 0; j < COLS; j++)
    {
        mvaddch(drone.starty - 2, j, ' ');
    }
    // printing the drones coordinates
    char msg[100];
    sprintf(msg, "The Drones Coordinates: (%05.2f, %.2f)", x, y);

    attron(A_BOLD);
    mvprintw(drone.starty - 2, (COLS - (int)strlen(msg)) / 2 + 1, "%s", msg);
    // printw("%s", msg);
}

// Draw drone within the structure
void draw_drone_at(float ee_x, float ee_y)
{
    // Clear the drone area
    for (int j = 0; j < drone.width; j++)
    {
        for (int i = 0; i < drone.height; i++)
        {
            mvaddch(drone.starty + i, drone.startx + j, ' ');
        }
    }
    // Convert real coordinates to integer
    int ee_x_int = floor(ee_x);
    int ee_y_int = floor(ee_y);

    // Draw the Drone
    attron(A_BOLD | COLOR_PAIR(4));
    mvaddch(drone.starty + ee_y_int, drone.startx + ee_x_int, '+');
    attroff(A_BOLD | COLOR_PAIR(4));

    refresh();
}

// Utility method to check for drone within limits
void check_ee_within_limits(float *ee_x, float *ee_y)
{

    // Checks for horizontal axis
    if (*ee_x <= 0)
    {
        *ee_x = 0;
    }
    else if (*ee_x >= DRONE_X_LIM)
    {
        *ee_x = DRONE_X_LIM - 1;
    }

    // Checks for vertical axis
    if (*ee_y <= 0)
    {
        *ee_y = 0;
    }
    else if (*ee_y >= DRONE_Y_LIM)
    {
        *ee_y = DRONE_Y_LIM - 1;
    }
}

// Create button windows
void make_buttons()
{

    int stp_button_startx = (COLS - 2 * BTN_SIZE - 5) / 2 + 5;
    int rst_button_startx = (COLS - BTN_SIZE + 11) / 2 + 5;
    int quit_button_startx = (COLS - 6 * BTN_SIZE) / 2 + 5;
    int buttons_starty = 35;

    stp_button = newwin(BTN_SIZE / 2, BTN_SIZE, buttons_starty, stp_button_startx);
    rst_button = newwin(BTN_SIZE / 2, BTN_SIZE, buttons_starty, rst_button_startx);
    quit_button = newwin(BTN_SIZE / 2, BTN_SIZE, buttons_starty, quit_button_startx);
}
// Draw button with colored background and custom color
void draw_btn(WINDOW *btn, char label, int color, int box_color)
{

    wbkgd(btn, COLOR_PAIR(box_color));
    wmove(btn, BTN_SIZE / 4, BTN_SIZE / 2);

    attron(A_BOLD | COLOR_PAIR(color));
    waddch(btn, label);
    attroff(A_BOLD | COLOR_PAIR(color));

    wrefresh(btn);
}
// Draw all buttons, prepending label message
void draw_buttons()
{

    char *msg = "Drone Simulator";
    move(2, (COLS - strlen(msg)) / 2);
    attron(A_BOLD);
    printw("%s", msg);
    attroff(A_BOLD);

    // draw_btn(stp_button, 'Z', 2, 2);  // 'S' with color pair 2 for text, 2 for box
    draw_btn(rst_button, 'D', 3, 3);  // 'R' with color pair 3 for text, 3 for box
    draw_btn(quit_button, 'Q', 2, 2); // 'Q' with color pair 1 for text, 4 for box
}
// Method to draw container (if set) withing hoist's workspace
void draw_obstacle()
{

    for (int i = 0; i < MAX_OBSTACLES; i++)
    {
        attron(A_BOLD | COLOR_PAIR(0));
        if (obstacle[i].value != 0)
        {
            mvaddch(obstacle[i].y + drone.starty, obstacle[i].x + drone.startx, 'O');
            attroff(A_BOLD | COLOR_PAIR(2));
        }
    }
}
// Method to draw target numbers
void draw_numbers()
{
    attron(A_BOLD | COLOR_PAIR(1)); // Use color pair 3 (yellow)

    for (int i = 0; i < MAX_NUMBERS; i++)
    {
        // Only print the number if its value is not 0
        if (number[i].value != 0)
        {
            mvprintw(number[i].y + drone.starty, number[i].x + drone.startx, "%d", number[i].value);
        }
    }

    attroff(A_BOLD | COLOR_PAIR(3));
}
// Method to delete the numbers
void delete_numbers(float ee_x, float ee_y)
{

    for (int i = 0; i < num_targets; i++)
    {
        if (fabs(ee_x - number[i].x) < 1 && fabs(ee_y - number[i].y) < 1)
        {
            // Drone is on the coordinates of the number, delete it
            number[i].value = 0;
        }
    }
}
// Method to check if all numbers values are zero
bool all_numbers_zero()
{
    for (int i = 0; i < MAX_TARG_ARR_SIZE; i++)
    {
        if (number[i].value != 0)
        {
            // Found a number with a non-zero value
            return false; // Not all numbers have a value of zero
        }
    }
    // All numbers have a value of zero
    return true;
}
// Method to update the obstacle motion
void update_obstacle_motion()
{
    int obstacles_fd;
    char *obstacles_fifo = "/tmp/obstacle_motion";
    mkfifo(obstacles_fifo, 0666);
    obstacles_fd = open(obstacles_fifo, O_WRONLY);
    for (int i = 0; i < num_obstacles; i++)
    {
        write(obstacles_fd, &obstacle[i], sizeof(obstacle[i]));
    }
    close(obstacles_fd);
}
// Method to update the numbers motion
void init_console_ui()
{

    // Initialize curses mode
    initscr();
    start_color();

    // Disable line buffering...
    cbreak();

    // Disable char echoing and blinking cursos
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(0);

    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_RED);
    init_pair(3, COLOR_BLACK, COLOR_YELLOW);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);

    // Initialize UI elements
    make_buttons();
    make_drone_field();

    // Set the initial position of the drone (replace these values we desire for the initial position)
    float initial_ee_x = 0;
    float initial_ee_y = 0;

    // initialize_numbers();

    // draw UI elements
    draw_drone_field();
    draw_buttons();
    draw_drone_msg(initial_ee_x, initial_ee_y);
    // draw_obstacle();
    draw_numbers();
    draw_drone_at(initial_ee_x, initial_ee_y);

    refresh();

    // Activate input listening (keybord  ...)
    keypad(stdscr, TRUE);
}
// Method to update the console UI
void update_console_ui(float *ee_x, float *ee_y)
{

    // check if next drone position is within limits
    check_ee_within_limits(ee_x, ee_y);

    // Draw updated drone position
    draw_drone_at(*ee_x, *ee_y);

    // Update string message for drone position
    draw_drone_msg(*ee_x, *ee_y);

    draw_obstacle();
    draw_numbers();
    delete_numbers(*ee_x, *ee_y);

    refresh();
}
// Method to reset the console UI
void reset_console_ui()
{

    // Free resources
    delwin(stp_button);
    delwin(rst_button);
    delwin(quit_button);

    // Clear screen
    erase();

    // Re-create UI elements
    make_drone_field();
    make_buttons();

    // draw UI elements
    draw_drone_field();
    draw_drone_msg(0, 0);
    draw_buttons();

    refresh();
}