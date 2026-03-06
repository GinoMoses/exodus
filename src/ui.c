#include <ncurses.h>
#include <unistd.h>
#include <string.h>

#include "memory.h"

// gotta split this file up at some point

#define FULL_BLOCK ACS_BLOCK

// unicode for borders
#define BOX_HORIZONTAL 0x2500
#define BOX_VERTICAL 0x2502
#define BOX_TOP_LEFT 0x250C
#define BOX_TOP_RIGHT 0x2510
#define BOX_BOTTOM_LEFT 0x2514
#define BOX_BOTTOM_RIGHT 0x2518

#define CPU_USAGE_COLOR(p) ((p) < 50.0 ? 1 : ((p) < 80.0 ? 2 : 3)) 
#define CPU_COLUMNS 4
#define PERCENT_WIDTH 7 
#define CELL_PADDING 2
#define START_Y 1
#define START_X 2

// window definitions
static WINDOW *cpu_window = NULL;
static WINDOW *memory_window = NULL;
static WINDOW *network_window = NULL;
static WINDOW *footer_window = NULL;

void shutdown_ui(void) {
    if (cpu_window) delwin(cpu_window);
    if (memory_window) delwin(memory_window);
    if (network_window) delwin(network_window);
    if (footer_window) delwin(footer_window);
    endwin();
}

static void create_cpu_window(size_t core_count) {
    int term_h, term_w;
    getmaxyx(stdscr, term_h, term_w);
    (void)term_h; // unused

    int rows = (core_count + CPU_COLUMNS - 1) / CPU_COLUMNS;
    int cpu_height = rows + 3;

    if (cpu_window) delwin(cpu_window);
    cpu_window = newwin(cpu_height, term_w - 4, START_Y, START_X);
}

static void create_memory_window(void) {
    int term_h, term_w;
    getmaxyx(stdscr, term_h, term_w);
    (void)term_h; // unused
    
    int cpu_win_h, cpu_win_w;
    getmaxyx(cpu_window, cpu_win_h, cpu_win_w);
    (void)cpu_win_w; // unused

    int memory_height = 5;
    int memory_y = START_Y + cpu_win_h + 1;
    int memory_width = (term_w - 4) / 2 - 1;

    if (memory_window) delwin(memory_window);
    memory_window = newwin(memory_height, memory_width, memory_y, START_X);
}

static void create_network_window(void) {
    int term_h, term_w;
    getmaxyx(stdscr, term_h, term_w);
    (void)term_h; // unused

    int cpu_win_h, cpu_win_w;
    getmaxyx(cpu_window, cpu_win_h, cpu_win_w);
    (void)cpu_win_w;

    int network_height = 5;
    int network_y = START_Y + cpu_win_h + 1;

    int network_width = (term_w - 4) / 2 - 1;
    int network_x = START_X + network_width + 2;

    if (network_window) delwin(network_window);
    network_window = newwin(network_height, network_width, network_y, network_x);
}

static void create_footer_window(void) {
    int term_h, term_w;
    getmaxyx(stdscr, term_h, term_w);

    int footer_height = 3;
    int footer_y = term_h - footer_height;

    if (footer_window) delwin(footer_window);
    footer_window = newwin(footer_height, term_w - 4, footer_y, START_X);
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
    
    // memory bar colors
    // reuse some from cpu
    init_pair(6, COLOR_CYAN, -1);
    init_pair(7, COLOR_MAGENTA, -1);

    refresh();
    getch();
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

static void draw_bar(WINDOW *win, double cpu_usage, int bar_width) {
    int filled = (int)(cpu_usage / 100.0 * bar_width);

    for (int i = 0; i < bar_width; i++) {
        double pos_percent = (double)i / bar_width * 100.0;
        int color = CPU_USAGE_COLOR(pos_percent);

        if (i < filled) {
            wattron(win, COLOR_PAIR(color));
            waddch(win, FULL_BLOCK);
            wattroff(win, COLOR_PAIR(color));
        } else {
            wattron(win, COLOR_PAIR(5));
            waddch(win, FULL_BLOCK);
            wattroff(win, COLOR_PAIR(5));
        }
    }
}

static void draw_memory_bar(WINDOW *win, const memory_stats_t *memory, int bar_width) {
    // seperate function for memory to color buffers, cached, and used differently
    double used_filled = memory->total > 0 ? 
        (int)((double)memory->used / memory->total * bar_width) : 0.0;
    double buffer_end = memory->total > 0 ? 
        (int)((double)(memory->used + memory->buffers) / memory->total * bar_width) : 0.0;
    double cache_end = memory->total > 0 ? 
        (int)((double)(memory->used + memory->buffers + memory->cached) / memory->total * bar_width) : 0.0;

    for (int i = 0; i < bar_width; i++) {
        if (i < used_filled) {
            wattron(win, COLOR_PAIR(6));
            waddch(win, FULL_BLOCK);
            wattroff(win, COLOR_PAIR(6));
        } else if (i < buffer_end) {
            wattron(win, COLOR_PAIR(7));
            waddch(win, FULL_BLOCK);
            wattroff(win, COLOR_PAIR(7));
        } else if (i < cache_end) {
            wattron(win, COLOR_PAIR(2));
            waddch(win, FULL_BLOCK);
            wattroff(win, COLOR_PAIR(2));
        } else {
            wattron(win, COLOR_PAIR(5));
            waddch(win, FULL_BLOCK);
            wattroff(win, COLOR_PAIR(5));
        }
    }
}

static void draw_swap_bar(WINDOW *win, unsigned long long swap_total, unsigned long long swap_used, int bar_width) {
    if (swap_total == 0) {
        wattron(win, COLOR_PAIR(5));
        
        for (int i = 0; i < bar_width; i++) {
            waddch(win, FULL_BLOCK);
        }
        
        wattroff(win, COLOR_PAIR(5));
        return;
    }

    int filled = swap_total > 0 ? (int)((double)swap_used / swap_total * bar_width) : 0;

    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            wattron(win, COLOR_PAIR(7));
            waddch(win, FULL_BLOCK);
            wattroff(win, COLOR_PAIR(7));
        } else {
            wattron(win, COLOR_PAIR(5));
            waddch(win, FULL_BLOCK);
            wattroff(win, COLOR_PAIR(5));
        }
    }
}

static void draw_titled_box(WINDOW *win, const char *title) {
    wattron(win, COLOR_PAIR(5));
    box(win, 0, 0);

    if (title) {
        mvwprintw(win, 0, 2, " %s ", title);
    }

    wattroff(win, COLOR_PAIR(5));
}

static void draw_cpu(double *cpu_usage, size_t core_count) {
    if (core_count == 0) return; 

    if (!cpu_window) create_cpu_window(core_count);

    draw_titled_box(cpu_window, "CPU");
    
    int win_h, win_w;
    getmaxyx(cpu_window, win_h, win_w);
    (void)win_h; // unused

    int label_width     = get_label_width(core_count);
    int fixed_width     = label_width + PERCENT_WIDTH + 1;
    int usable_width    = win_w - 4 - (CPU_COLUMNS - 1) * CELL_PADDING;
    int column_width    = usable_width / CPU_COLUMNS;
    int bar_width       = column_width - fixed_width;

    if (bar_width < 0) bar_width = 0;

    int rows = (core_count + CPU_COLUMNS - 1) / CPU_COLUMNS;

    for (size_t i = 0; i < core_count; i++) {
        int col = i / rows;
        int row = i % rows;

        int y = 1 + row;
        int x = 2 + col * (column_width + CELL_PADDING);

        int label_x = x;
        int bar_x = label_x + label_width + 1;
        int percent_x = bar_x + bar_width + 1;

        wattron(cpu_window, COLOR_PAIR(4));
        mvwprintw(cpu_window, y, label_x, "%*zu", label_width, i+1);
        wattroff(cpu_window, COLOR_PAIR(4));
        
        wmove(cpu_window, y, bar_x);
        draw_bar(cpu_window, cpu_usage[i], bar_width);

        if (percent_x + PERCENT_WIDTH <= win_w) {
            mvwprintw(cpu_window, y, percent_x, "%6.2f%%", cpu_usage[i]);
        }
    }
}

static void format_memory(unsigned long long kb, char *buffer, size_t buffer_size) {
    if (kb >= 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.1fG", (double)kb / (1024 * 1024));
    } else if (kb >= 1024) {
        snprintf(buffer, buffer_size, "%.1fM", (double)kb / 1024);
    } else {
        snprintf(buffer, buffer_size, "%lluK", kb);
    }
}

static void draw_memory(const memory_stats_t *memory) { 
    if (!memory || memory->total == 0) return;

    if (!memory_window) create_memory_window();

    draw_titled_box(memory_window, "MEM");

    int win_h, win_w;
    getmaxyx(memory_window, win_h, win_w);
    (void)win_h; // unused

    const char *label   = "MEM";
    int usable_width    = win_w - 4;
    int label_width     = strlen(label);
    
    char used_str[16], total_str[16];
    format_memory(memory->used, used_str, sizeof(used_str));
    format_memory(memory->total, total_str, sizeof(total_str));

    int mem_info_width = 15;
    int bar_brackets = 2;

    int bar_width = usable_width - label_width - 1 - mem_info_width - bar_brackets - 1;
    if (bar_width < 20) bar_width = 20;
    
    int y = 1; 
    int x = 2;

    wattron(memory_window, COLOR_PAIR(4));
    mvwprintw(memory_window, y, x, "%s", label);
    wattroff(memory_window, COLOR_PAIR(4));

    int bar_x = x + label_width + 1;

    wmove(memory_window, y, bar_x);
    draw_memory_bar(memory_window, memory, bar_width);

    int info_x = bar_x + bar_brackets + bar_width + 1;
    mvwprintw(memory_window, y, info_x, "%s / %s", used_str, total_str);

    y++;

    char swap_used_str[16], swap_total_str[16];
    format_memory(memory->swap_used, swap_used_str, sizeof(swap_used_str));
    format_memory(memory->swap_total, swap_total_str, sizeof(swap_total_str));
    
    const char *swap_label = "SWP";
    wattron(memory_window, COLOR_PAIR(4));
    mvwprintw(memory_window, y, x, "%s", swap_label);
    wattroff(memory_window, COLOR_PAIR(4));

    int swap_bar_x = x + strlen(swap_label) + 1;
    wmove(memory_window, y, swap_bar_x);
    draw_swap_bar(memory_window, memory->swap_total, memory->swap_used, bar_width);

    if (memory->swap_total == 0) {
        wattron(memory_window, COLOR_PAIR(3));
        mvwprintw(memory_window, y, info_x, "no swap");
        wattroff(memory_window, COLOR_PAIR(3));
    } else {
        mvwprintw(memory_window, y, info_x, "%s / %s", swap_used_str, swap_total_str);
    }
}

static void draw_network(void) {
    if (!network_window) create_network_window();

    box(network_window, 0, 0);
    draw_titled_box(network_window, "NET");
    mvwprintw(network_window, 2, 2, "Coming soon");
}

static void draw_footer(void) {
    if (!footer_window) {
        create_footer_window();
        box(footer_window, 0, 0);
        mvwprintw(footer_window, 1, 2, "q: Quit");
        wnoutrefresh(footer_window);
    }
}

void update_ui(double *cpu_usage, size_t core_count, const memory_stats_t *memory) {
    draw_cpu(cpu_usage, core_count);
    draw_memory(memory);
    draw_network();
    draw_footer();

    wnoutrefresh(cpu_window);
    wnoutrefresh(memory_window);
    wnoutrefresh(network_window);
    // wnoutrefresh(footer_window); - not needed, content doesn't change
    doupdate();
}
