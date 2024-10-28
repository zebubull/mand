#include "viewport.h"

#include <sys/ioctl.h>
#include <unistd.h>

static struct winsize get_target_size() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if (w.ws_row % 2 == 0) {
        w.ws_row -= 1;
    }
    return w;
}

struct viewport vp_init(struct vec2 center, struct vec2 extents) {
    struct winsize con_size = get_target_size();
    struct viewport vp = {
        .rows = con_size.ws_row,
        .cols = con_size.ws_col,
        .center = center,
        .extents = extents,
    };

    /* Edit the extents of the viewport to force the correct aspect ratio */
    double height = vp.extents.y * 2;
    /* Each cell is 2.5 times taller than it is wide */
    double height_ratio = height / vp.rows / 2.5;
    double width = vp.cols * height_ratio;
    vp.extents.x = width / 2;
    
    vp_update_cellsize(&vp);
    return vp;
}

void vp_update_cellsize(struct viewport *vp) {
    vp->cell_size.x = vp->extents.x * 2 / (double)vp->cols;
    vp->cell_size.y = vp->extents.y * 2 / (double)vp->rows;
}

void vp_shift(struct viewport *vp, enum DIR direction, int cells) {
    struct vec2 new_center = vp->center;
    switch (direction) {
    case DIR_LEFT:
        new_center.x -= cells * vp->cell_size.x;
        break;
    case DIR_RIGHT:
        new_center.x += cells * vp->cell_size.x;
        break;
    case DIR_UP:
        new_center.y += cells * vp->cell_size.y;
        break;
    case DIR_DOWN:
        new_center.y -= cells * vp->cell_size.y;
        break;
    }

    vp_move(vp, new_center);
}

void vp_move(struct viewport *vp, struct vec2 new_center) {
    vp->center = new_center;
}

#define ZOOM_FACTOR 0.05

void vp_zoom(struct viewport *vp, enum ZOOM zoom) {
    double zoom_factor = 1.0;

    switch (zoom) {
        /* zooming in shrinks the viewport, so the zoom here is inverted */
    case ZOOM_IN:
        zoom_factor -= ZOOM_FACTOR;
        break;
    case ZOOM_OUT:
        zoom_factor += ZOOM_FACTOR;
        break;
    }

    vp->extents.x *= zoom_factor;
    vp->extents.y *= zoom_factor;
    vp_update_cellsize(vp);
}
