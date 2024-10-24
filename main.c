#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "term.h"
#include "util.h"
#include "viewport.h"

char *screen_buffer;

bool in_mandelbrot_set(struct complex c) {
    double i;
    struct complex z = { .real = 0.0, .imag = 0.0 };

    for (i = 0; i < 1.0; i += 0.005) {
        complex_square(&z);
        complex_add(&z, &c);
        if (complex_mag(z) > 4.0) {
            return false;
        }
    }
    return true;
}

void draw_cell(double x, double y, int index) {
    struct complex c = { .real = x, .imag = y };
    char sym = ' ';
    if (in_mandelbrot_set(c)) {
        sym = '*';
    }
    screen_buffer[index] = sym;
}

void draw_mandelbrot(struct viewport *vp) {
    int row, col;
    double x, y, top, left;
    top = vp->center.y + vp->extents.y;
    left = vp->center.x - vp->extents.x;

    for (row = 0, y = top; row < vp->rows; ++row, y -= vp->cell_size.y) {
        for (col = 0, x = left; col < vp->cols; ++col, x += vp->cell_size.x) {
            draw_cell(x, y, row * vp->cols + col);
        }
    }
    move_cursor(1, 1);
    write(STDOUT_FILENO, screen_buffer, vp->rows * vp->cols * sizeof(char));
    move_cursor(vp->rows / 2, vp->cols / 2);
}

struct viewport create_initial_viewport() {
    struct vec2 center = { .x = -0.5, .y = 0.0 };
    struct vec2 extents = { .x = 1.5, .y = 1.0};
    return vp_init(center, extents);
}

int main() {
    // Initial setup
    enter_raw_mode();
    struct viewport vp = create_initial_viewport();
    screen_buffer = malloc(vp.rows * vp.cols * sizeof(char));

    bool running = true;
    while (running) {
        draw_mandelbrot(&vp);
        int c = getchar();
        switch (c) {
            case 'q': running = 0; break;

            case 'h': vp_shift(&vp, DIR_LEFT, 1); break;
            case 'j': vp_shift(&vp, DIR_DOWN, 1); break;
            case 'k': vp_shift(&vp, DIR_UP, 1); break;
            case 'l': vp_shift(&vp, DIR_RIGHT, 1); break;
            case 'H': vp_shift(&vp, DIR_LEFT, 5); break;
            case 'J': vp_shift(&vp, DIR_DOWN, 5); break;
            case 'K': vp_shift(&vp, DIR_UP, 5); break;
            case 'L': vp_shift(&vp, DIR_RIGHT, 5); break;

            case 'i': vp_zoom(&vp, ZOOM_IN); break;
            case 'o': vp_zoom(&vp, ZOOM_OUT); break;
            case 'I': vp_zoom(&vp, ZOOM_IN); vp_zoom(&vp, ZOOM_IN); break;
            case 'O': vp_zoom(&vp, ZOOM_OUT); vp_zoom(&vp, ZOOM_OUT); break;
        }
    }

    free(screen_buffer);
    
    return 0;
}
