#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "term.h"
#include "util.h"
#include "viewport.h"

char *screen_buffer;
int   buffer_bytes;

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

#define CELL_TL (1 << 0)
#define CELL_TR (1 << 1)
#define CELL_BL (1 << 2)
#define CELL_BR (1 << 3)

#define BLOCK_ALL "█"
#define BLOCK_TOP "▀"
#define BLOCK_BOT "▄"
#define BLOCK_TL  "▘"
#define BLOCK_TR  "▝"
#define BLOCK_BL  "▖"
#define BLOCK_BR  "▗"
#define BLOCK_LFT "▌"
#define BLOCK_RGT "▐"
#define BLOCK_TLC "▛"
#define BLOCK_BLC "▙"
#define BLOCK_TRC "▜"
#define BLOCK_BRC "▟"
#define BLOCK_BLD "▞"
#define BLOCK_TLD "▚"

static const char *marching_symbols[] = {
    "",
    BLOCK_TL,
    BLOCK_TR,
    BLOCK_TOP,
    BLOCK_BL,
    BLOCK_LFT,
    BLOCK_BLD,
    BLOCK_TLC,
    BLOCK_BR,
    BLOCK_TLD,
    BLOCK_RGT,
    BLOCK_TRC,
    BLOCK_BOT,
    BLOCK_BLC,
    BLOCK_BRC,
    BLOCK_ALL,
};

static const char *cell_name(int cell) {
    switch (cell) {
        case 0: return "empty";
        case CELL_BL: return "bl";
        case CELL_BR: return "br";
        case CELL_TL: return "tl";
        case CELL_TR: return "tr";
        case CELL_BL | CELL_BR: return "bottom";
        case CELL_BL | CELL_TL: return "left";
        case CELL_TL | CELL_TR: return "top";
        case CELL_TR | CELL_BR: return "right";
        case CELL_BL | CELL_TR: return "bl-tr";
        case CELL_TL | CELL_BR: return "tl-br";
        case CELL_BL | CELL_BR | CELL_TL: return "bot-left";
        case CELL_BL | CELL_BR | CELL_TR: return "bot-right";
        case CELL_TL | CELL_TR | CELL_BL: return "top-left";
        case CELL_TL | CELL_TR | CELL_BR: return "top-right";
        case CELL_TL | CELL_TR | CELL_BL | CELL_BR: return "all";
        default: return "unknown";
    }
}

static void debug_syms() {
    for (int i = 1; i < 16; ++i) {
        printf("%-16s: %s\n", cell_name(i), marching_symbols[i]);
    }
    exit(0);
}

void draw_cell_march(double left, double top, double x_step, double y_step) {
    char cell_state = 0000;
    double cx, cy;
    int y, x;

    cy = top;
    for (y = 0; y < 2; ++y) {
        cx = left;
        for (x = 0; x < 2; ++x) {
            struct complex c = { .real = cx, .imag = cy };
            if (in_mandelbrot_set(c)) {
                cell_state |= (1 << (y * 2 + x));
            }
            cx += x_step / 2;
        }
        cy += y_step / 2;
    }

    if (!cell_state) {
        screen_buffer[buffer_bytes] = ' ';
        buffer_bytes++;
    } else {
        memcpy(screen_buffer + buffer_bytes, marching_symbols[cell_state], 3);
        buffer_bytes += 3;
    }
}

void draw_cell(double x, double y) {
    struct complex c = { .real = x, .imag = y };
    char sym = ' ';
    if (in_mandelbrot_set(c)) {
        sym = '*';
    }
    screen_buffer[buffer_bytes] = sym;
    ++buffer_bytes;
}

void draw_mandelbrot(struct viewport *vp) {
    int row, col;
    double x, y, top, left;
    top = vp->center.y + vp->extents.y - vp->cell_size.y / 2;
    left = vp->center.x - vp->extents.x + vp->cell_size.x / 2;
    buffer_bytes = 0;

    for (row = 0, y = top; row < vp->rows; ++row, y -= vp->cell_size.y) {
        for (col = 0, x = left; col < vp->cols; ++col, x += vp->cell_size.x) {
            #ifdef MARCHING
            draw_cell_march(x, y,
                            vp->cell_size.x, vp->cell_size.y);
            #else
            draw_cell(x, y);
            #endif
        }
    }
    move_cursor(1, 1);
    write(STDOUT_FILENO, screen_buffer, buffer_bytes);
    move_cursor(vp->rows / 2, vp->cols / 2);
}

struct viewport create_initial_viewport() {
    struct vec2 center = { .x = -0.5, .y = 0.0 };
    struct vec2 extents = { .x = 1.5, .y = 1.0};
    return vp_init(center, extents);
}

int main() {
    // Initial setup
    // debug_syms();
    enter_raw_mode();
    struct viewport vp = create_initial_viewport();
    screen_buffer = malloc(vp.rows * vp.cols * sizeof(int));

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
