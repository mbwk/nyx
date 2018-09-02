#include "term.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "strbuf.h"

void editor_clear_screen();

void die(const char *s)
{
    struct strbuf sb;
    editor_clear_screen(&sb);
    write(STDOUT_FILENO, sb.b, sb.len);
    sb_free(&sb);
    perror(s);
    exit(1);
}

static struct editor_config *active;
void exit_raw_mode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &active->orig_termios) == -1) {
        die("tcsetattr");
    }
}

void enter_raw_mode(struct editor_config *E)
{
    struct termios muta_termios;
    if (tcgetattr(STDIN_FILENO, &E->orig_termios) == -1) {
        die("tcgetattr");
    }

    active = E;
    atexit(exit_raw_mode);

    muta_termios = E->orig_termios;
    muta_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    muta_termios.c_oflag &= ~(OPOST);
    muta_termios.c_cflag |= (CS8);
    muta_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    muta_termios.c_cc[VMIN] = 0;
    muta_termios.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &muta_termios) == -1) {
        die("tcsetattr");
    }
}

binding terminal_read_key()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b') {  // escape sequence
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return BIND_HOME;
                        case '3': return BIND_DEL;
                        case '4': return BIND_END;
                        case '5': return BIND_PAGE_UP;
                        case '6': return BIND_PAGE_DOWN;
                        case '7': return BIND_HOME;
                        case '8': return BIND_END;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return BIND_ARROW_UP;
                    case 'B': return BIND_ARROW_DOWN;
                    case 'C': return BIND_ARROW_RIGHT;
                    case 'D': return BIND_ARROW_LEFT;
                    case 'H': return BIND_HOME;
                    case 'F': return BIND_END;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return BIND_HOME;
                case 'F': return BIND_END;
            }
        }

        return '\x1b';
    } else {  // plain keypress
        switch (c) {
            case CTRL_KEY('q'):
                return BIND_QUIT;
        }
    }
    return c;
}

int get_cursor_position(int *rows, int *cols)
{
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        ++i;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

int get_window_size(int *rows, int *cols)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return get_cursor_position(rows, cols);
        return -1;
    }

    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
}

