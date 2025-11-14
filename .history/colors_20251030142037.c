// Runtime implementation for color helpers used by the game
#include "colors.h"

// Map a NULL window to stdscr for convenience
static inline WINDOW *win_or_stdscr(WINDOW *win) {
    return win ? win : stdscr;
}

int has_color_support(void) {
    return has_colors();
}

int init_colors(void) {
    if (!has_colors()) {
        return 0;
    }
    if (start_color() == ERR) {
        return 0;
    }

    // Initialize pairs. Choose contrasting, readable defaults.
    // Fallback if COLOR_ constants are limited in some terminals.
    init_pair(COLOR_SNAKE_HEAD,    COLOR_YELLOW,   COLOR_BLACK);
    init_pair(COLOR_SNAKE_BODY,    COLOR_WHITE,    COLOR_BLACK);
    init_pair(COLOR_FOOD,          COLOR_GREEN,      COLOR_BLACK);
    init_pair(COLOR_TEMP_FOOD,     COLOR_CYAN,     COLOR_BLACK);
    init_pair(COLOR_SPECIAL_ITEM,  COLOR_MAGENTA,  COLOR_BLACK);
    init_pair(COLOR_BORDER,        COLOR_WHITE,    COLOR_BLACK);
    init_pair(COLOR_TEXT,          COLOR_WHITE,    COLOR_BLACK);
    init_pair(COLOR_ERROR,         COLOR_RED,      COLOR_BLACK);
    init_pair(COLOR_SUCCESS,       COLOR_,    COLOR_BLACK);
    init_pair(COLOR_WARNING,       COLOR_YELLOW,   COLOR_BLACK);
    init_pair(COLOR_DEATH_ITEM,    COLOR_RED,      COLOR_BLACK);

    return 1;
}

void cleanup_colors(void) {
    // Nothing specific to do for PDCurses/ncurses — kept for symmetry
}

static inline void apply_attr(WINDOW *win, int attr) {
    WINDOW *w = win_or_stdscr(win);
    wattron(w, attr);
}

static inline void clear_attrs(WINDOW *win) {
    WINDOW *w = win_or_stdscr(win);
    wattrset(w, A_NORMAL);
}

void apply_snake_head_color(WINDOW *win)    { apply_attr(win, ATTR_SNAKE_HEAD); }
void apply_snake_body_color(WINDOW *win)    { apply_attr(win, ATTR_SNAKE_BODY); }
void apply_food_color(WINDOW *win)          { apply_attr(win, ATTR_FOOD); }
void apply_temp_food_color(WINDOW *win)     { apply_attr(win, ATTR_TEMP_FOOD); }
void apply_special_item_color(WINDOW *win)  { apply_attr(win, ATTR_SPECIAL_ITEM); }
void apply_death_item_color(WINDOW *win)    { apply_attr(win, ATTR_DEATH_ITEM); }
void apply_border_color(WINDOW *win)        { apply_attr(win, ATTR_BORDER); }
void apply_text_color(WINDOW *win)          { apply_attr(win, ATTR_TEXT); }
void apply_error_color(WINDOW *win)         { apply_attr(win, ATTR_ERROR); }
void apply_success_color(WINDOW *win)       { apply_attr(win, ATTR_SUCCESS); }
void apply_warning_color(WINDOW *win)       { apply_attr(win, ATTR_WARNING); }

void remove_all_colors(WINDOW *win) {
    clear_attrs(win);
}