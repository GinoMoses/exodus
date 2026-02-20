#include <ncurses.h>
#include <unistd.h>

// call endwin() without including ncurses.h in main
void shutdown_ui() {
    endwin();
}

int initialize_ui(double *cpu_usage) {
    initscr();
    cbreak();
    noecho();
    curs_set(false);
    nodelay(stdscr, true);
    
    mvprintw(0, 0, "CPU Usage: %.2f%%", *cpu_usage);
    refresh();
    usleep(100000); 
    
    return 0;
}
