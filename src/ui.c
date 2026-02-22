#include <ncurses.h>
#include <unistd.h>
#include <string.h>

#include "memory.h"

#define CPU_USAGE_COLOR(p) ((p) < 50.0 ? 1 : ((p) < 80.0 ? 2 : 3)) 
#define CPU_COLUMNS 4
#define PERCENT_WIDTH 7 
#define BAR_BRACKETS 2
#define CELL_PADDING 2
#define START_Y 1
#define START_X 2

// call endwin() without including ncurses.h in main
void shutdown_ui(void) {
    endwin();
}

void initialize_ui(void) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, true);

    start_color();
    use_default_colors();

    // cpu bar colors
    init_pair(1, COLOR_GREEN, -1);
    init_pair(2, COLOR_YELLOW, -1);
    init_pair(3, COLOR_RED, -1);

    // generic ui colors
    init_pair(4, COLOR_BLUE, -1);
    if (COLORS >= 256) init_pair(5, 245, -1); // gray
    
}

static int get_label_width(size_t core_count) {
    int digits = 1;
    size_t max = core_count - 1;

    while (max >= 10) {
        max /= 10;
        digits++;
    }

    return digits + 1;
}

static void draw_bar(int y, int x, double cpu_usage, int bar_width) {
    int filled = (int)(cpu_usage / 100.0 * bar_width);

    attron(COLOR_PAIR(5));
    mvaddch(y, x, '[');
    attroff(COLOR_PAIR(5));
    
    for (int i = 0; i < bar_width; i++) {
        double pos_percent = (double)i / bar_width * 100.0;
        int color = CPU_USAGE_COLOR(pos_percent);

        attron(COLOR_PAIR(color));
        addch(i < filled ? '=' : ' ');
        attroff(COLOR_PAIR(color));
    }

    attron(COLOR_PAIR(5));
    addch(']');
    attroff(COLOR_PAIR(5));
}

int draw_cpu(double *cpu_usage, size_t core_count) {
    erase();

    int term_h, term_w;
    getmaxyx(stdscr, term_h, term_w); 
    (void)term_h; // unused

    if (core_count == 0) return 0;

    // can't do terminal UI for shit lmao
    int label_width     = get_label_width(core_count);
    int fixed_width     = label_width + PERCENT_WIDTH + BAR_BRACKETS + 1;
    int usable_width    = term_w - START_X - (CPU_COLUMNS - 1) * CELL_PADDING;
    int column_width    = usable_width / CPU_COLUMNS;
    int bar_width       = column_width - fixed_width; 

    if (bar_width < 0) bar_width = 0;

    int rows = (core_count + CPU_COLUMNS - 1) / CPU_COLUMNS;

    for (size_t i = 0; i < core_count; i++) {
        int col = i / rows;
        int row = i % rows;

        int y = START_Y + row;
        int x = START_X + col * column_width;
      
        int label_x = x; 
        int bar_x = label_x + label_width;
        int percent_x = bar_x + BAR_BRACKETS + bar_width + 1;  
        
        attron(COLOR_PAIR(4));
        mvprintw(y, label_x, "%*zu", label_width, i+1);
        attroff(COLOR_PAIR(4));
        draw_bar(y, bar_x, cpu_usage[i], bar_width);
        if (percent_x + PERCENT_WIDTH <= term_w) {
            mvprintw(y, percent_x, "%6.2f%%", cpu_usage[i]);
    
        }
    }

    refresh();
    return rows;
}

void draw_memory(const memory_stats_t *memory, int cpu_rows) { 
    if (!memory || memory->total == 0) return;

    int term_h, term_w;
    getmaxyx(stdscr, term_h, term_w);
    (void)term_h; // unused

    int y = START_Y + cpu_rows + 1;

    double percent = memory->total > 0 ? (double)memory->used / memory->total * 100.0 : 0.0;

    int usable_width = term_w - START_X * 2;
    
    const char *label = "MEM";
    int label_width = strlen(label);
    
    int fixed_width = label_width + 1 + PERCENT_WIDTH + BAR_BRACKETS + 1;
    int bar_width = usable_width - fixed_width;

    if (bar_width < 0) bar_width = 0;

    int x = START_X;
    int bar_x = x + label_width + 1;
    int percent_x = bar_x + BAR_BRACKETS + bar_width + 1;

    attron(COLOR_PAIR(4));
    mvprintw(y, x, "%s", label);
    attroff(COLOR_PAIR(4));
    draw_bar(y, bar_x, percent, bar_width);
    if (percent_x + PERCENT_WIDTH <= term_w) {
        mvprintw(y, percent_x, "%6.2f%%", percent);
    }

    refresh();
}

