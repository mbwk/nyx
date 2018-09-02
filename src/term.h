#pragma once

#include <termios.h>
#include <time.h>

#define CTRL_KEY(k) ((k) & 0x1f)

#define TAB_STOP 8

typedef struct erow {
    int size;
    int rsize;
    char *chars;
    char *render;
} erow;

struct editor_config {
    int cx, cy;
    int rx;
    int row_off;
    int col_off;
    int screen_rows;
    int screen_cols;
    int num_rows;
    erow *row;
    char *filename;
    char statusmsg[80];
    time_t statusmsg_time;
    struct termios orig_termios;
};

enum editor_binding {
    BIND_QUIT = 1000,
    BIND_ARROW_LEFT,
    BIND_ARROW_RIGHT,
    BIND_ARROW_UP,
    BIND_ARROW_DOWN,
    BIND_DEL,
    BIND_HOME,
    BIND_END,
    BIND_PAGE_UP,
    BIND_PAGE_DOWN
};

typedef int binding;

void die(const char *s);
void exit_raw_mode();
void enter_raw_mode(struct editor_config *E);
binding terminal_read_key();
int get_cursor_position(int *rows, int *cols);
int get_window_size(int *rows, int *cols);

