/*
 * nyx editor
 * */

#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "strbuf.h"
#include "term.h"


#define NYX_VERSION "0.0.1"

static struct editor_config E;

/* row operations */

int editor_row_cx_to_rx(erow *row, int cx)
{
    int rx = 0;
    int j;
    for (j = 0; j < cx; ++j) {
        if (row->chars[j] == '\t') {
            rx += (TAB_STOP - 1) - (rx % TAB_STOP);
        }
        ++rx;
    }
    return rx;
}

void editor_update_row(erow *row)
{
    int tabs = 0;
    int j;
    for (j = 0; j < row->size; ++j) {
        if (row->chars[j] == '\t') ++tabs;
    }

    free(row->render);
    row->render = malloc(row->size + tabs * (TAB_STOP - 1) + 1);

    int idx = 0;
    for (j = 0; j < row->size; ++j) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % TAB_STOP != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}

void editor_append_row(char *s, size_t len)
{
    E.row = realloc(E.row, sizeof(erow) * (E.num_rows + 1));

    int at = E.num_rows;
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';

    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    editor_update_row(E.row + at);

    E.num_rows++;
}


/* file i/o */

void editor_open(char *filename) {
    free(E.filename);
    E.filename = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                               line[linelen - 1] == '\r'))
            --linelen;

        editor_append_row(line, linelen);
    }
    free(line);
    fclose(fp);
}

/* output */

void editor_scroll()
{
    E.rx = 0;
    if (E.cy < E.num_rows) {
        E.rx = editor_row_cx_to_rx(E.row + E.cy, E.cx);
    }

    if (E.cy < E.row_off) {
        E.row_off = E.cy;
    }
    if (E.cy >= E.row_off + E.screen_rows) {
        E.row_off = E.cy - E.screen_rows + 1;
    }
    if (E.rx < E.col_off) {
        E.col_off = E.rx;
    }
    if (E.rx >= E.col_off + E.screen_cols) {
        E.col_off = E.rx - E.screen_cols + 1;
    }
}

void editor_draw_rows(struct strbuf *sb)
{
    int y;
    for (y = 0; y < E.screen_rows; ++y) {
        int file_row = y + E.row_off;
        if (file_row >= E.num_rows) {
            if (E.num_rows == 0 && y == E.screen_rows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                    "nyx editor -- version %s", NYX_VERSION);
                if (welcomelen > E.screen_cols) welcomelen = E.screen_cols;
                int padding = (E.screen_cols - welcomelen) / 2;
                if (padding) {
                    sb_append(sb, "~", 1);
                    --padding;
                }
                while (padding--) sb_append(sb, " ", 1);
                sb_append(sb, welcome, welcomelen);
            } else {
                sb_append(sb, "~", 1);
            }
        } else {
            int len = E.row[file_row].rsize - E.col_off;
            if (len < 0) len = 0;
            if (len > E.screen_cols) len = E.screen_cols;
            sb_append(sb, E.row[file_row].render + E.col_off, len);
        }

        sb_append(sb, "\x1b[K", 3);
        sb_append(sb, "\r\n", 2);
    }
}

void editor_draw_status_bar(struct strbuf *sb)
{
    sb_append(sb, "\x1b[7m", 4);
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines",
                       E.filename ? E.filename : "[No Name]", E.num_rows);
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d  ",
                        E.cy + 1, E.num_rows);
    if (len > E.screen_cols) len = E.screen_cols;
    sb_append(sb, status, len);
    while (len < E.screen_cols) {
        if (E.screen_cols == rlen + len) {
            sb_append(sb, rstatus, rlen);
            break;
        } else {
            sb_append(sb, " ", 1);
            ++len;
        }
    }
    sb_append(sb, "\x1b[m", 3);
}

void editor_clear_screen(struct strbuf *sb)
{
    sb_append(sb, "\x1b[2J", 4);
    sb_append(sb, "\x1b[H", 3);
}

void editor_refresh_screen()
{
    if (get_window_size(&E.screen_rows, &E.screen_cols) == -1) die("get_window_size");
    E.screen_rows -= 1;

    editor_scroll();

    struct strbuf sb = STRBUF_INIT;

    sb_append(&sb, "\x1b[?25l", 6);
    sb_append(&sb, "\x1b[H", 3);

    editor_draw_rows(&sb);
    editor_draw_status_bar(&sb);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH",
             (E.cy - E.row_off) + 1,
             (E.rx - E.col_off) + 1);
    sb_append(&sb, buf, strlen(buf));

    //sb_append(&sb, "\x1b[H", 3);
    sb_append(&sb, "\x1b[?25h", 6);

    write(STDOUT_FILENO, sb.b, sb.len);
    sb_free(&sb);
}


/* input */
#define CURRENT_ROW ( (E.cy >= E.num_rows ? NULL : E.row + E.cy) )

void editor_move_cursor(binding key)
{
    erow *row = CURRENT_ROW;

    switch (key) {
        case BIND_ARROW_LEFT:
            if (E.cx > 0) {
                E.cx--;
            } else if (E.cy > 0) {
                E.cx = E.row[--E.cy].size;
            }
            break;
        case BIND_ARROW_RIGHT:
            if (row && E.cx < row->size) {
                E.cx++;
            } else if (row && E.cx == row->size) {
                E.cy++;
                E.cx = 0;
            }
            break;
        case BIND_ARROW_UP:
            if (E.cy > 0) {
                E.cy--;
            }
            break;
        case BIND_ARROW_DOWN:
            if (E.cy < E.num_rows) {
                E.cy++;
            }
            break;
    }

    row = CURRENT_ROW;
    int row_len = row ? row->size : 0;
    if (E.cx > row_len) {
        E.cx = row_len;
    }
}

void editor_process_keypress()
{
    binding c = terminal_read_key();

    struct strbuf sb = STRBUF_INIT;
    switch (c) {
        case BIND_QUIT:
            editor_clear_screen(&sb);
            write(STDOUT_FILENO, sb.b, sb.len);
            sb_free(&sb);
            exit(0);
            break;

        case BIND_HOME:
            E.cx = 0;
            break;

        case BIND_END:
            if (E.cy < E.num_rows) {
                E.cx = E.row[E.cy].size;
            }
            break;

        case BIND_PAGE_UP:
        case BIND_PAGE_DOWN:
            {
                if (c == BIND_PAGE_UP) {
                    E.cy = E.row_off;
                } else if (c == BIND_PAGE_DOWN) {
                    E.cy = E.row_off + E.screen_rows - 1;
                    if (E.cy > E.num_rows) E.cy = E.num_rows;
                }

                int times = E.screen_rows;
                while (times--)
                    editor_move_cursor(c == BIND_PAGE_UP ?
                                       BIND_ARROW_UP :
                                       BIND_ARROW_DOWN);
            }
            break;

        case BIND_ARROW_UP:
        case BIND_ARROW_DOWN:
        case BIND_ARROW_LEFT:
        case BIND_ARROW_RIGHT:
            editor_move_cursor(c);
            break;
    }
}

void init_editor(struct editor_config *econf)
{
    econf->cx = 0;
    econf->cy = 0;
    econf->rx = 0;
    econf->row_off = 0;
    econf->col_off = 0;
    econf->num_rows = 0;
    econf->row = NULL;
    econf->filename = NULL;
    econf->statusmsg[0] = '\0';
    econf->statusmsg_time = 0;

    if (get_window_size(&econf->screen_rows, &econf->screen_cols) == -1) die("get_window_size");
    econf->screen_rows -= 1;
}

int main(int argc, char *argv[])
{
    enter_raw_mode(&E);
    init_editor(&E);
    if (argc >= 2) {
        editor_open(argv[1]);
    }

    while (1) {
        editor_refresh_screen();
        editor_process_keypress();
    }
    return 0;
}

