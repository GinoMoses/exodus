#include <ncurses.h>
#include <unistd.h>

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

    init_pair(1, COLOR_GREEN, -1);
    init_pair(2, COLOR_YELLOW, -1);
    init_pair(3, COLOR_RED, -1);
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

    mvaddch(y, x, '[');
    
    for (int i = 0; i < bar_width; i++) {
        double pos_percent = (double)i / bar_width * 100.0;
        int color = CPU_USAGE_COLOR(pos_percent);

        attron(COLOR_PAIR(color));
        addch(i < filled ? '=' : ' ');
        attroff(COLOR_PAIR(color));
    }

    addch(']');
}

void draw_cpu(double *cpu_usage, size_t core_count) {
    erase();

    int term_w;
    getmaxyx(stdscr, term_w, term_w); 

    if (core_count == 0) return;

    // can't do terminal UI for shit lmao
    int label_width = get_label_width(core_count);
    int fixed_width = label_width + PERCENT_WIDTH + BAR_BRACKETS + 1;
    int column_width = term_w / CPU_COLUMNS;
    int bar_width = column_width - fixed_width; 

    if (bar_width < 1) bar_width = 1;

    int rows = (core_count + CPU_COLUMNS - 1) / CPU_COLUMNS;

    for (size_t i = 0; i < core_count; i++) {
        int col = i / rows;
        int row = i % rows;

        int y = START_Y + row;
        int x = START_X + col * column_width;
        
        mvprintw(y, x, "%*zu", label_width, i);
        draw_bar(y, x + label_width, cpu_usage[i], bar_width);
        mvprintw(y, x + label_width + bar_width + BAR_BRACKETS + 1, "%6.2f%%", cpu_usage[i]);
    }
    
    refresh();
}
