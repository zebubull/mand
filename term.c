#include "term.h"

#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>

struct termios orig_term;

void disable_raw_mode() {
    /* Leave alternate screen */
    write(STDOUT_FILENO, "\e[?1049l", sizeof("\e[?1049l"));
    fflush(stdout);
    /* Restore canonical mode */
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
}

void enter_raw_mode() {
    /* Get original term attributes */
    tcgetattr(STDIN_FILENO, &orig_term);
    atexit(disable_raw_mode);

    struct termios raw = orig_term;
    /* Disable echoing and read bytes instead of lines */
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    /* Enter alternate screen */
    write(STDOUT_FILENO, "\e[?1049h", sizeof("\e[?1049h"));
    fflush(stdout);
}

void clear_term() {
    write(STDOUT_FILENO, "\e[2J", sizeof("\e[2J"));
    printf("\e[1;1H");
    fflush(stdout);
}

void move_cursor(int row, int col) {
    printf("\e[%d;%dH", row, col);
    fflush(stdout);
}
