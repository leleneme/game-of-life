#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/select.h>
#include <termios.h>
#include <string.h>
#include <png.h>

static struct termios orig_termios;
static int g_generation;

static int width = 20;
static int height = 20;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    struct termios new_termios;

    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    atexit(disable_raw_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit() {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

int getch() {
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0)
        return r;
    else
        return c;
}

void populate_rand(int width, int height, int universe[width][height]) {
    g_generation = 0;
    for (int x = 0; x < width; x++)
        for (int y = 0; y < height; y++)
            universe[x][y] = rand() < RAND_MAX / 10 ? 1 : 0;
}

void display(int width, int height, int universe[width][height]) {
    printf("\033[2J \033[H");
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) 
            printf(universe[x][y] ? "\033[07m  \033[m" : "  ");

        printf("\033[E");
    }
    printf("p: pause, r: reset, q: quit    gen. %d", g_generation);
    fflush(stdout);
}

void update(int width, int height, int universe[width][height]) {
    int new[height][width];
 
    for (int x = 0; x < width; x++)
        for (int y = 0; y < height; y++) {
            int n = 0;
            for (int x1 = x - 1; x1 <= x + 1; x1++)
                for (int y1 = y - 1; y1 <= y + 1; y1++)
                    if (universe[(x1 + width) % width][(y1 + height) % height])
                        n++;
    
            if (universe[x][y]) n--;
            new[x][y] = (n == 3 || (n == 2 && universe[x][y]));
        }
    for (int x = 0; x < width; x++)
        for (int y = 0; y < height; y++)
        universe[x][y] = new[x][y];

    g_generation++;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    if (argc == 3) {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
    }
    
    int universe[width][height];

    populate_rand(width, height, universe);
    enable_raw_mode();
    printf("\033[2J\033[H");

    unsigned time_bt = 100000;
    int paused  = 0;
    int running = 1;
    while(running) {
        while (!kbhit())
            if (!paused){
                display(width, height, universe);
                update(width, height, universe);
                usleep(time_bt);
            }

        switch (getch()) {
            case 'q':
            case 'Q':
                running = 0;
                break;
            case 'r':
            case 'R':
                populate_rand(width, height, universe);
                break;
            case 'p':
            case 'P':
                paused = paused ? 0 : 1;
                break;
            default:
                break;
        }
    }

    printf("\033[2J \033[H");
    return 0;
}