#include <ncurses.h>

#include "input.h"

int handle_input(void) {
    int ch = getch();

    switch (ch) {
        case 'q':
        case 'Q':
              return INPUT_QUIT;
        default:
              return INPUT_NONE;
    }
    
    return 0;
}
