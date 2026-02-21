#include <ncurses.h>
#include <unistd.h>

// call endwin() without including ncurses.h in main
void shutdown_ui() {
    endwin();
}

int initialize_ui(double *cpu_usage, size_t *core_count) {
    initscr();
    cbreak();
    noecho();
    curs_set(false);
    nodelay(stdscr, true);
   
    mvprintw(0, 0, "CPU Usage Monitor (Press 'q' to quit)");

    for (size_t i = 0; i < *core_count; i++) {
        mvprintw(i+1, 0, "CPU %zu: %.2f%%", i, cpu_usage[i]);
    }

    refresh();
    
    return 0;
}
