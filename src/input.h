#ifndef INPUT_H
#define INPUT_H

typedef enum {
    INPUT_QUIT,
    INPUT_NONE,
    INPUT_ARROW_UP,
    INPUT_ARROW_DOWN,
    INPUT_PAGE_UP,
    INPUT_PAGE_DOWN,
    INPUT_KILL,
    INPUT_SORT_NEXT,
    INPUT_SORT_PREV
} input_action_t;

int handle_input(void);

#endif // INPUT_H
