#include <ncurses.h>

#include "input.h"

int handle_input(void) {
    int ch = getch();

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
        
        default:
            return INPUT_NONE;
    }
    
    return 0;
}
