#ifndef COLORS_H
#define COLORS_H

#ifdef _WIN32
    #include "PDCurses/curses.h"  // Use local PDCurses headers on Windows
#else
    #include <ncurses.h> // ncurses for Linux/Mac
#endif

// Color pair definitions
#define COLOR_SNAKE_HEAD    1
#define COLOR_SNAKE_BODY    2
#define COLOR_FOOD          3
#define COLOR_TEMP_FOOD     4
#define COLOR_SPECIAL_ITEM  5
#define COLOR_BORDER        6
#define COLOR_TEXT          7
#define COLOR_ERROR         8
#define COLOR_SUCCESS       9
#define COLOR_WARNING       10
#define COLOR_DEATH_ITEM    11

// Attribute combinations
#define ATTR_SNAKE_HEAD     (COLOR_PAIR(COLOR_SNAKE_HEAD) | A_BOLD)
#define ATTR_SNAKE_BODY     (COLOR_PAIR(COLOR_SNAKE_BODY))
#define ATTR_FOOD           (COLOR_PAIR(COLOR_FOOD) | A_BOLD)
#define ATTR_TEMP_FOOD      (COLOR_PAIR(COLOR_TEMP_FOOD) | A_BOLD)
#define ATTR_SPECIAL_ITEM   (COLOR_PAIR(COLOR_SPECIAL_ITEM) | A_BOLD)
#define ATTR_BORDER         (COLOR_PAIR(COLOR_BORDER) | A_BOLD)
#define ATTR_TEXT           (COLOR_PAIR(COLOR_TEXT))
#define ATTR_ERROR          (COLOR_PAIR(COLOR_ERROR) | A_BOLD)
#define ATTR_SUCCESS        (COLOR_PAIR(COLOR_SUCCESS) | A_BOLD)
#define ATTR_WARNING        (COLOR_PAIR(COLOR_WARNING) | A_BOLD)
#define ATTR_DEATH_ITEM     (COLOR_PAIR(COLOR_DEATH_ITEM) | A_BOLD)

// Function prototypes
int init_colors(void);
void cleanup_colors(void);
int has_color_support(void);

// Convenience functions for applying colors
void apply_snake_head_color(WINDOW *win);
void apply_snake_body_color(WINDOW *win);
void apply_food_color(WINDOW *win);
void apply_temp_food_color(WINDOW *win);
void apply_special_item_color(WINDOW *win);
void apply_death_item_color(WINDOW *win);
void apply_border_color(WINDOW *win);
void apply_text_color(WINDOW *win);
void apply_error_color(WINDOW *win);
void apply_success_color(WINDOW *win);
void apply_warning_color(WINDOW *win);

// Functions to turn off colors
void remove_all_colors(WINDOW *win);

#endif // COLORS_H