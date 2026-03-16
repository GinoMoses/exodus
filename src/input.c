#include <ncurses.h>

#include "input.h"


static char last_char = '\0';

int handle_input(void) {
    int ch = getch();
    last_char = '\0';

    switch (ch) {
        case 'q':
        case 'Q':
            return INPUT_QUIT;

        case KEY_UP:
            return INPUT_ARROW_UP;
        
        case KEY_DOWN:
            return INPUT_ARROW_DOWN;
        
        case KEY_PPAGE:
            return INPUT_PAGE_UP;
        
        case KEY_NPAGE:
            return INPUT_PAGE_DOWN;
            
        case 'k':
        case 'K':
            return INPUT_KILL;

        case KEY_RIGHT:
        case '>':
        case '.':
            return INPUT_SORT_NEXT;

        case KEY_LEFT:
        case '<':
        case ',':
            return INPUT_SORT_PREV;

        case '/':
            return INPUT_FILTER;

        case 27: // ESC
             return INPUT_FILTER_CANCEL;

        case '\n':
        case '\r':
        case KEY_ENTER:
            return INPUT_FILTER_ACCEPT;

        case KEY_BACKSPACE:
        case 127:
        case 8:
            return INPUT_FILTER_BACKSPACE;

        case ERR:
            return INPUT_NONE;

        default:
            if (ch >= 32 && ch <= 126) {
                last_char = (char)ch;
                return INPUT_FILTER_CHAR;
            }
            return INPUT_NONE;
    }
    
    return 0;
}

char input_get_last_char(void) {
    return last_char;
}
