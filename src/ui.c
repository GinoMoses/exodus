#include <ncurses.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <locale.h>

#include "memory.h"
#include "network.h"
#include "system.h"
#include "process.h"
#include "process_compare.h"

// gotta split this file up at some point

#define FULL_BLOCK ACS_BLOCK
#define GRAPH_BLOCK ACS_CKBOARD

#define DOWN_ARROW ACS_DARROW
#define UP_ARROW ACS_UARROW

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

#define NETWORK_HISTORY_MAX 120

static double *download_history = NULL;
static double *upload_history = NULL;
static double max_download_recorded = 0;
static double max_upload_recorded = 0;
static int history_size = 0;
static int history_index = 0;
static int history_count = 0;
static network_stats_t previous_network_stats = {0};
static int network_stats_initialized = 0;

static int process_select_index = 0;
static int process_scroll_offset = 0;

// window definitions
static WINDOW *cpu_window = NULL;
static WINDOW *memory_window = NULL;
static WINDOW *network_window = NULL;
static WINDOW *process_window = NULL;
static WINDOW *footer_window = NULL;

typedef enum {
    // maintain order of which they appear in the UI
    SORT_PID,
    SORT_USER,
    SORT_CPU,
    SORT_MEM,
    SORT_COMMAND
} process_sort_t;

static process_sort_t current_process_sort = SORT_PID;

void shutdown_ui(void) {
    if (cpu_window) delwin(cpu_window);
    if (memory_window) delwin(memory_window);
    if (network_window) delwin(network_window);
    if (process_window) delwin(process_window);
    if (footer_window) delwin(footer_window);
    
    if (download_history) free(download_history);
    if (upload_history) free(upload_history);

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

    int memory_height = 12;
    int memory_y = START_Y + cpu_win_h + 1;
    
    int available_width = term_w - 4;
    int gap = 2;
    int memory_width = (available_width - gap) / 2;

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

    int network_height = 12;
    int network_y = START_Y + cpu_win_h + 1;
    
    int mem_win_w;
    getmaxyx(memory_window, network_height, mem_win_w);

    int available_width = term_w - 4;
    int gap = 2;

    int network_x = START_X + mem_win_w + gap;
    int network_width = available_width - mem_win_w - gap;

    if (network_window) delwin(network_window);
    network_window = newwin(network_height, network_width, network_y, network_x);
}

static void create_process_window(void) {
    int term_h, term_w;
    getmaxyx(stdscr, term_h, term_w);
    
    int mem_win_h, mem_win_w;
    getmaxyx(memory_window, mem_win_h, mem_win_w);
    (void)mem_win_w; // unused

    int cpu_win_h, cpu_win_w;
    getmaxyx(cpu_window, cpu_win_h, cpu_win_w);
    (void)cpu_win_w; // unused

    int proc_y = 1 + cpu_win_h + mem_win_h + 2;
    int proc_height = term_h - proc_y - 4;
    int proc_width = term_w - 4;

    if (process_window) delwin(process_window);
    process_window = newwin(proc_height, proc_width, proc_y, START_X);
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
    setlocale(LC_ALL, "");

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    start_color();
    assume_default_colors(-1, COLOR_BLACK);
    use_default_colors();

    // color scheme
    if (COLORS >= 256) {
        // cpu
        init_pair(1, 41, -1);
        init_pair(2, 185, -1);
        init_pair(3, 208, -1);
        init_pair(4, 77, -1);
        init_pair(6, 157, -1);

        // memory
        init_pair(7, 75, -1);
        init_pair(8, 39, -1);
        init_pair(9, 33, -1);
        init_pair(10, 27, -1);

        // network
        init_pair(11, 170, -1);
        init_pair(12, 141, -1);

        // process
        init_pair(13, 51, -1);

        init_pair(5, 237, -1);
    } else {
        // whatever man
        init_pair(1, COLOR_GREEN, -1);
        init_pair(2, COLOR_YELLOW, -1);
        init_pair(3, COLOR_RED, -1);
        init_pair(4, COLOR_BLUE, -1);
        init_pair(5, COLOR_BLACK, -1);
        init_pair(6, COLOR_CYAN, -1);
        init_pair(7, COLOR_MAGENTA, -1);
        init_pair(8, COLOR_GREEN, -1);
        init_pair(9, COLOR_GREEN, -1);
        init_pair(10, COLOR_GREEN, -1);
        init_pair(11, COLOR_MAGENTA, -1);
        init_pair(12, COLOR_CYAN, -1);
    }

    refresh();
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
            if (COLORS >= 256) {
                init_pair(20, 39, -1);
                wattron(win, COLOR_PAIR(8));
            } else {
                wattron(win, COLOR_PAIR(8));
            }
            waddch(win, FULL_BLOCK);
            wattroff(win, COLOR_PAIR(COLORS >= 256 ? 20 : 4));
        } else if (i < buffer_end) {
            if (COLORS >= 256) {
                init_pair(21, 33, -1);
                wattron(win, COLOR_PAIR(9));
            } else {
                wattron(win, COLOR_PAIR(9));
            }
            waddch(win, FULL_BLOCK);
            wattroff(win, COLOR_PAIR(COLORS >= 256 ? 21 : 6));
        } else if (i < cache_end) {
            if (COLORS >= 256) {
                init_pair(22, 27, -1);
                wattron(win, COLOR_PAIR(10));
            } else {
                wattron(win, COLOR_PAIR(10));
            }
            waddch(win, FULL_BLOCK);
            wattroff(win, COLOR_PAIR(COLORS >= 256 ? 22 : 4));
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

static void draw_titled_box(WINDOW *win, const char *title, int color_pair) {
    wattron(win, COLOR_PAIR(color_pair));
    box(win, 0, 0);

    if (title) {
        wattron(win, A_BOLD);
        mvwprintw(win, 0, 2, " %s ", title);
        wattroff(win, A_BOLD);
    }
    wattroff(win, COLOR_PAIR(5));
}

static void draw_cpu(double *cpu_usage, size_t core_count) {
    if (core_count == 0) return; 

    if (!cpu_window) create_cpu_window(core_count);

    draw_titled_box(cpu_window, "CPU", 4);
    
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

        wattron(cpu_window, COLOR_PAIR(6));
        mvwprintw(cpu_window, y, label_x, "%*zu", label_width, i);
        wattroff(cpu_window, COLOR_PAIR(6));
        
        wmove(cpu_window, y, bar_x);
        draw_bar(cpu_window, cpu_usage[i], bar_width);

        if (percent_x + PERCENT_WIDTH <= win_w) {
            wattron(cpu_window, COLOR_PAIR(6));
            mvwprintw(cpu_window, y, percent_x, "%6.2f%%", cpu_usage[i]);
            wattroff(cpu_window, COLOR_PAIR(6));
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

static void draw_memory(const memory_stats_t *memory, const system_stats_t *system) { 
    if (!memory || memory->total == 0) return;

    if (!memory_window) create_memory_window();

    draw_titled_box(memory_window, "MEM", 7);

    int win_h, win_w;
    getmaxyx(memory_window, win_h, win_w);
    (void)win_h; // unused

    const char *label = "MEM";
    int usable_width = win_w - 4;
    int label_width = strlen(label);
    
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
        char swap_used_str[16], swap_total_str[16];
        format_memory(memory->swap_used, swap_used_str, sizeof(swap_used_str));
        format_memory(memory->swap_total, swap_total_str, sizeof(swap_total_str));
        mvwprintw(memory_window, y, info_x, "%s / %s", swap_used_str, swap_total_str);
    }

    if (system) {
        y += 2;

        long days = system->uptime_seconds / 86400;
        long hours = (system->uptime_seconds % 86400) / 3600;
        long minutes = (system->uptime_seconds % 3600) / 60;

        mvwprintw(memory_window, y++, x, "Uptime: %ldd %ldh %ldm", days, hours, minutes);
        mvwprintw(memory_window, y++, x, "Load: %.2f %.2f %2.f",
                system->load_1min, system->load_5min, system->load_15min);
        mvwprintw(memory_window, y++, x, "Procs: %d", system->process_count);
        mvwprintw(memory_window, y++, x, "Kernel: %s", system->kernel_version);
    }
}

static void init_network_history(int size) {
    if (download_history) {
        free(download_history);
        free(upload_history);
    }

    history_size = size;
    if (history_size > NETWORK_HISTORY_MAX) history_size = NETWORK_HISTORY_MAX;
    if (history_size < 10) history_size = 10;

    download_history = calloc(history_size, sizeof(double));
    upload_history = calloc(history_size, sizeof(double));
    history_index = 0;
}

static void update_network_history(const network_stats_t *current) {
    static int first_calc = 1;

    if (!network_stats_initialized) {
        previous_network_stats = *current;
        network_stats_initialized = 1;
        
        if (download_history && upload_history) {
            for (int i = 0; i < history_size; i++) {
                download_history[i] = 0;
                upload_history[i] = 0;
            }
        }

        return;
    }

    double download_speed = (double)(current->rx_bytes - previous_network_stats.rx_bytes);
    double upload_speed = (double)(current->tx_bytes - previous_network_stats.tx_bytes);

    // skip first sample
    if (first_calc) {
        first_calc = 0;
        previous_network_stats = *current;
        return;
    }

    download_history[history_index] = download_speed;
    upload_history[history_index] = upload_speed;

    if (download_speed > max_download_recorded) max_download_recorded = download_speed;
    if (upload_speed > max_upload_recorded) max_upload_recorded = upload_speed;

    history_index = (history_index + 1) % history_size;
    if (history_count < history_size) history_count++;

    previous_network_stats = *current;
}

static void format_speed(double bytes_per_sec, char *buffer, size_t buffer_size) {
    if (bytes_per_sec >= 1024 * 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.1fGB/s", bytes_per_sec / (1024 * 1024 * 1024));
    } else if (bytes_per_sec >= 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.1fMB/s", bytes_per_sec / (1024 * 1024));
    } else  if (bytes_per_sec >= 1024) {
        snprintf(buffer, buffer_size, "%.1fKB/s", bytes_per_sec / 1024);
    } else {
        snprintf(buffer, buffer_size, "%.0fB/s", bytes_per_sec);
    }
}

static void draw_network(const network_stats_t *network) {
    if (!network_window) {
        create_network_window();
    }
   
    char title[64];
    
    if (network && network->name[0] != '\0') {
        sprintf(title, "NET [%s]", network->name);
    }

    draw_titled_box(network_window, title, 11);
    
    int win_h, win_w;
    getmaxyx(network_window, win_h, win_w);
   
    int graph_width = win_w - 4;

    if (!download_history || history_size != graph_width) {
        init_network_history(graph_width);
    }

    int current_idx = (history_index - 1 + history_size) % history_size;
    double current_download = download_history[current_idx];
    double current_upload = upload_history[current_idx];
    
    char down_str[16], up_str[16];
    format_speed(current_download, down_str, sizeof(down_str));
    format_speed(current_upload, up_str, sizeof(up_str));
    
    double max_download = 0.0001;
    double max_upload = 0.0001;
    
    for (int i = 0; i < history_size; i++) {
        if (download_history[i] > max_download) max_download = download_history[i];
        if (upload_history[i] > max_upload) max_upload = upload_history[i];
    }
    
    int center_y = win_h / 2;
    int graph_height = (win_h - 4) / 2;
    int graph_x_start = 2;
    
    wattron(network_window, COLOR_PAIR(12));
    wmove(network_window, 1, 2);
    waddch(network_window, DOWN_ARROW);
    wprintw(network_window, " %-10s", down_str);
    wattroff(network_window, COLOR_PAIR(12));
   
    char peak_download_str[16];
    format_speed(max_download_recorded, peak_download_str, sizeof(peak_download_str));
    wattron(network_window, COLOR_PAIR(12));
    wprintw(network_window, " peak: %-10s", peak_download_str);
    wattroff(network_window, COLOR_PAIR(12));

    wattron(network_window, COLOR_PAIR(11));
    wmove(network_window, win_h - 2, 2);
    waddch(network_window, UP_ARROW);
    wprintw(network_window, " %-10s", up_str);
    wattroff(network_window, COLOR_PAIR(11));

    char peak_upload_str[16];
    format_speed(max_upload_recorded, peak_upload_str, sizeof(peak_upload_str));
    wattron(network_window, COLOR_PAIR(11));
    wprintw(network_window, " peak: %-10s", peak_upload_str);
    wattroff(network_window, COLOR_PAIR(11));
    
    for (int i = 0; i < history_size && i < graph_width; i++) {
        int history_idx = (history_index + i) % history_size;
        
        double down_ratio = pow(download_history[history_idx] / max_download, 0.33);
        double up_ratio = pow(upload_history[history_idx] / max_upload, 0.33);

        int down_height = (int)(down_ratio * graph_height);
        int up_height = (int)(up_ratio * graph_height);
        
        int x = graph_x_start + i;

        int has_data = (i >= history_size - history_count);

        for (int y = 0; y < graph_height; y++) {
            if (y < down_height) {
                wattron(network_window, COLOR_PAIR(12));
                mvwaddch(network_window, center_y - y - 1, x, GRAPH_BLOCK);
                wattroff(network_window, COLOR_PAIR(12));
            } else if (has_data && y == 0) {
                wattron(network_window, COLOR_PAIR(12));
                mvwaddch(network_window, center_y - y - 1, x, ACS_S9);
                wattroff(network_window, COLOR_PAIR(12));
            } else {
                mvwaddch(network_window, center_y - y - 1, x, ' ');
            }
        }
        
        for (int y = 0; y < graph_height; y++) {
            if (y < up_height) {
                wattron(network_window, COLOR_PAIR(11));
                mvwaddch(network_window, center_y + y, x, GRAPH_BLOCK);
                wattroff(network_window, COLOR_PAIR(11));
            } else if (has_data && y == 0) {
                wattron(network_window, COLOR_PAIR(11));
                mvwaddch(network_window, center_y + y, x, ACS_S1);
                wattroff(network_window, COLOR_PAIR(11));
            } else {
                mvwaddch(network_window, center_y + y, x, ' ');
            }
        } 
    }
}

void cycle_sort_mode(int direction) {
    if (direction > 0) {
        current_process_sort = (current_process_sort + 1) % 5;
    } else if (direction < 0) {
        current_process_sort = (current_process_sort - 1 + 5) % 5;
    }
}

const char* get_sort_mode_name(void) {
    switch (current_process_sort) {
        case SORT_CPU: return "CPU%";
        case SORT_MEM: return "MEM%";
        case SORT_PID: return "PID";
        case SORT_USER: return "USER";
        case SORT_COMMAND: return "COMMAND";
        default: return "PID"; // always assume PID as default
    }
}

static void sort_process(process_list_t *processes) {
    if (!processes || processes->count == 0) return;

    int (*compare_func)(const void *, const void *) = NULL;

    switch (current_process_sort) {
        case SORT_CPU:
            compare_func = compare_by_cpu;
            break;
        case SORT_MEM:
            compare_func = compare_by_memory;
            break;
        case SORT_PID:
            compare_func = compare_by_pid;
            break;
        case SORT_USER:
            compare_func = compare_by_user;
            break;
        case SORT_COMMAND:
            compare_func = compare_by_command;
            break;
    }

    if (compare_func) {
        qsort(processes->processes, processes->count, sizeof(process_t), compare_func);
    }
}

int kill_process(const process_list_t *processes) {
    if (!processes || processes->count <= 0) return -1;
    if (process_select_index < 0 || process_select_index >= (int)processes->count) return -1;

    const process_t *proc = &processes->processes[process_select_index];

    if (kill(proc->pid, SIGTERM) == 0) {
        return 1;
    } else {
        return -1;
    }
}

void scroll_process_list(int delta) {
    process_select_index += delta;
}

static void draw_processes(const process_list_t *processes) {
    if (!process_window) create_process_window();
   
    werase(process_window);

    char title[32];
    snprintf(title, sizeof(title), "PROC [%s]", get_sort_mode_name());
    draw_titled_box(process_window, title, 13);
    
    int win_h, win_w;
    getmaxyx(process_window, win_h, win_w);
   
    if (processes && processes->count > 0) {
        sort_process((process_list_t *)processes);
    }

    wattron(process_window, A_BOLD | COLOR_PAIR(9));
    mvwprintw(process_window, 1, 2, "%-7s %-10s %3s %3s %6s %6s  %s",
            "PID", "USER", "PR", "NI", "CPU%", "MEM%", "COMMAND");
    wattroff(process_window, A_BOLD | COLOR_PAIR(9));

    if (!processes || processes->count == 0) {
        mvwprintw(process_window, 3, 2, "No processes found");
        process_select_index = 0;
        process_scroll_offset = 0;
        return;
    }

    if (process_select_index < 0) {
        process_select_index = 0;
    }
    if (process_select_index >= (int)processes->count) {
        process_select_index = (int)processes->count - 1;
    }

    int max_rows = win_h - 3;
    
    if (process_select_index < process_scroll_offset) {
        process_scroll_offset = process_select_index;
    }
    if (process_select_index >= process_scroll_offset + max_rows) {
        process_scroll_offset = process_select_index - max_rows + 1;
    }

    int max_scroll = (int)processes->count - max_rows;
    if (max_scroll < 0) max_scroll = 0;
    if (process_scroll_offset > max_scroll) {
        process_scroll_offset = max_scroll;
    }
    if (process_scroll_offset < 0) {
        process_scroll_offset = 0;
    }

    for (int i = 0; i < max_rows; i++) {
        int proc_idx = process_scroll_offset + i;
        if (proc_idx >= (int)processes->count) break;

        const process_t *proc = &processes->processes[proc_idx];

        int cmd_width = win_w - 52;
        char cmd_truncated[256];
        strncpy(cmd_truncated, proc->command, sizeof(cmd_truncated) -1);
        cmd_truncated[sizeof(cmd_truncated) - 1] = '\0';

        if ((int)strlen(cmd_truncated) > cmd_width) {
            cmd_truncated[cmd_width - 3] = '.';
            cmd_truncated[cmd_width - 2] = '.';
            cmd_truncated[cmd_width - 1] = '.';
            cmd_truncated[cmd_width] = '\0';
        }

        if (proc_idx == process_select_index) {
            wattron(process_window, A_REVERSE);
        }

        mvwprintw(process_window, 2 + i, 2, "%-7d %-10s %3ld %3ld %6.1f %6.1f  %s",
                proc->pid, proc->user, proc->priority, proc->nice, 
                proc->cpu_percent, proc->mem_percent, cmd_truncated);

        if (proc_idx == process_select_index) {
            int current_x = getcurx(process_window);
            for (int x = current_x; x < win_w - 2; x++) {
                waddch(process_window, ' ');
            }
            wattroff(process_window, A_REVERSE);
        }
    }

    if (max_scroll > 0) {
        int indicator_y = win_h - 1;
        mvwprintw(process_window, indicator_y, win_w - 20, "(%d-%d of %zu)",
                process_scroll_offset + 1,
                process_scroll_offset + max_rows < (int)processes->count ?
                    process_scroll_offset + max_rows : (int)processes->count,
                processes->count);
    }
}

void refresh_process_window(const process_list_t *processes) {
    draw_processes(processes);
    wnoutrefresh(process_window);
    doupdate();
}

static void draw_footer(void) {
    if (!footer_window) {
        create_footer_window();
        box(footer_window, 0, 0);
        mvwprintw(footer_window, 1, 2, "[q]: Quit  k: Kill Process  [");
        waddch(footer_window, ACS_UARROW);
        waddch(footer_window, ' ');
        waddch(footer_window, ACS_DARROW);
        wprintw(footer_window, "]: Scroll Process List  ");
        wprintw(footer_window, "[< >]: Change Sort  ");
        wnoutrefresh(footer_window);
    }
}

void update_ui(double *cpu_usage, size_t core_count, const memory_stats_t *memory, 
        const network_stats_t *network, const system_stats_t *system,
        const process_list_t *processes) {
    if (network) {
        update_network_history(network);
    }

    draw_cpu(cpu_usage, core_count);
    draw_memory(memory, system);
    draw_network(network);
    draw_processes(processes);
    draw_footer();

    wnoutrefresh(cpu_window);
    wnoutrefresh(memory_window);
    wnoutrefresh(network_window);
    wnoutrefresh(process_window);
    wnoutrefresh(footer_window);
    doupdate();
}
