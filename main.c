#include <math.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>

struct vec {
    double x, y;
};

double vmag(struct vec *c) {
    return hypotf(c->x, c->y);
}

void vsquare(struct vec *c) {
    double x = c->x, y = c->y;
    c->x = x * x - y * y;
    c->y = 2 * x * y;
}

void vadd(struct vec *a, struct vec *b) {
    a->x += b->x;
    a->y += b->y;
}

struct viewport {
    int rows, cols;
    double x_step, y_step;
    double left, right, top, bottom;
    double zoom;
};

int in_set(struct vec point) {
    double i;

    struct vec z = { .x = 0.0, .y = 0.0 };
    for (i = 0; i < 1.0; i += 0.005) {
        vsquare(&z);
        vadd(&z, &point);
        if (vmag(&z) > 4.0) {
            return i;
        }
    }

    return i;
}

#define X_LEFT   -2.0
#define X_RIGHT   0.5
#define Y_TOP     1.0
#define Y_BOTTOM  -1.0

void viewport_update_steps(struct viewport *vp) {
    vp->y_step = (vp->top - vp->bottom) / (double)vp->rows;
    vp->x_step = (vp->right - vp->left) / (double)vp->cols;
}

struct viewport get_viewport() {
    struct winsize w;
    struct viewport vp = { .left = X_LEFT, .right = X_RIGHT, .top = Y_TOP, .bottom = Y_BOTTOM, .zoom = 1.0 };

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    w.ws_row -= 1;
    if (w.ws_row % 2 == 1) {
        w.ws_row -= 1;
    }
    vp.rows = w.ws_row;
    vp.cols = w.ws_col;

    viewport_update_steps(&vp);

    return vp;
}

void draw_fractal(struct viewport *vp) {
    int row, col;
    double x, y;
    write(STDOUT_FILENO, "\e[2J", sizeof("\e[2J"));
    printf("\e[1;1H");
    fflush(stdout);

    // For each pair of (row,column), map it to an imaginary number
    for (row = 0, y = vp->top; row < vp->rows; row += 1, y -= vp->y_step) {
        // Round y=0 to correctly display the tail
        if (fabs(y) < 0.00001) {
            y = 0.0;
        }
        for (col = 0, x = vp->left; col < vp->cols; col += 1, x += vp->x_step) {
            struct vec z = { .x = x, .y = y };
            float border = in_set(z);
            if (border == 1.0) {
                printf("#");
            }
            else if (border > 0.9) {
                printf(":");
            } else {
                printf(" ");
            }
        }
    }

    printf("\e[%d;%dH", vp->rows / 2, vp->cols / 2);
}

enum DIR {
    DIR_LEFT,
    DIR_RIGHT,
    DIR_UP,
    DIR_DOWN,
};

enum ZOOM {
    ZOOM_IN,
    ZOOM_OUT
};

#define MOVE_SPEED 20

void viewport_move(struct viewport *vp, enum DIR dir) {
    switch (dir) {
    case DIR_LEFT:
    {
        int shift_cols = vp->cols / MOVE_SPEED;
        // Shift the viewport by the appropriate number of columns
        double move_amount = shift_cols * vp->x_step;
        vp->left -= move_amount;
        vp->right -= move_amount;
    } break;
    case DIR_RIGHT:
    {
        int shift_cols = vp->cols / MOVE_SPEED;
        // Shift the viewport by the appropriate number of columns
        double move_amount = shift_cols * vp->x_step;
        vp->left += move_amount;
        vp->right += move_amount;
    } break;
    case DIR_UP:
    {
        int shift_rows= vp->rows/ MOVE_SPEED;
        // Shift the viewport by the appropriate number of rows
        double move_amount = shift_rows* vp->y_step;
        vp->top += move_amount;
        vp->bottom += move_amount;
    } break;
    case DIR_DOWN:
    {
        int shift_rows= vp->rows/ MOVE_SPEED;
        // Shift the viewport by the appropriate number of rows
        double move_amount = shift_rows* vp->y_step;
        vp->top -= move_amount;
        vp->bottom -= move_amount;
    } break;
    }
}

#define zoom_speed 0.05

struct vec viewport_center(struct viewport *vp) {
    struct vec center = {
        .x = (vp->right + vp->left) / 2.0,
        .y = (vp->top + vp->bottom) / 2.0,
    };
    return center;
}

void viewport_zoom(struct viewport *vp, enum ZOOM zoom) {
    double zoom_factor = 1.0;
    struct vec center = viewport_center(vp);

    switch (zoom) {
    case ZOOM_IN:
    {
        double old_zoom = vp->zoom;
        // Zoom in by zoom_speed percent
        vp->zoom *= 1.0 + zoom_speed;
        // Zoom in shrinks the viewport so get the reciprocal of the change in zoom
        zoom_factor = old_zoom / vp->zoom;
    } break;
    case ZOOM_OUT:
    {
        double old_zoom = vp->zoom;
        // Zoom out by zoom_speed percent
        vp->zoom *= 1.0 - zoom_speed;
        // Zoom out grows the viewport so get the reciprocal of the change in zoom
        zoom_factor = old_zoom / vp->zoom;
    } break;
    }

    // Adjust the viewport
    float width = vp->right - vp->left;
    float height = vp->top - vp->bottom;

    float new_width = width * zoom_factor;
    float new_height = height * zoom_factor;

    vp->left = center.x - new_width / 2.0;
    vp->right = center.x + new_width / 2.0;
    vp->top = center.y + new_height / 2.0;
    vp->bottom = center.y - new_height / 2.0;
    viewport_update_steps(vp);

    // Move the center of the viewport so that it stays in place
    // struct vec new_center = viewport_center(vp);
    // if (new_center.x != old_center.x) {
    //     double offset = old_center.x - new_center.x;
    //     vp->left += offset;
    //     vp->right += offset;
    // }
    // if (new_center.y != old_center.y) {
    //     double offset = old_center.y - new_center.y;
    //     vp->top += offset;
    //     vp->bottom += offset;
    // }

    // struct vec fixed_center = viewport_center(vp);

    // printf("old: %f %f\n", old_center.x, old_center.y);
    // printf("new: %f %f\n", new_center.x, new_center.y);
    // printf("fixed: %f %f\n", fixed_center.x, fixed_center.y);
}

struct termios orig_term;

void disable_raw_mode() {
    // Leave alternate screen
    write(STDOUT_FILENO, "\e[?1049l", sizeof("\e[?1049l"));
    fflush(stdout);
    // Restore canonical mode
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
}

void enter_raw_mode() {
    // Get original term attributes
    tcgetattr(STDIN_FILENO, &orig_term);
    atexit(disable_raw_mode);

    struct termios raw = orig_term;
    // Disable echoing and read bytes instead of lines
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // Enter alternate screen
    write(STDOUT_FILENO, "\e[?1049h", sizeof("\e[?1049h"));
    fflush(stdout);
}

int main() {
    // Initial setup
    enter_raw_mode();
    struct viewport vp = get_viewport();
    
    int running = 1;
    while (running) {
        draw_fractal(&vp);
        int c = getchar();
        switch (c) {
            case 'q': running = 0; break;
            case 'h': viewport_move(&vp, DIR_LEFT); break;
            case 'j': viewport_move(&vp, DIR_DOWN); break;
            case 'k': viewport_move(&vp, DIR_UP); break;
            case 'l': viewport_move(&vp, DIR_RIGHT); break;
            case 'i': viewport_zoom(&vp, ZOOM_IN); break;
            case 'o': viewport_zoom(&vp, ZOOM_OUT); break;
        }
    }
    
    return 0;
}
