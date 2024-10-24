#pragma once
#include "util.h"

enum DIR {
    DIR_LEFT,
    DIR_RIGHT,
    DIR_UP,
    DIR_DOWN,
};

enum ZOOM {
    ZOOM_IN,
    ZOOM_OUT,
};

struct viewport {
    /* The number of rows and columns in the console */
    int rows, cols;
    /* The center of the viewport */
    struct vec2 center;
    /* The distance from the center of the viewport to
       the edge. In other words, half the width and height. */
    struct vec2 extents;
    struct vec2 cell_size;
};

/*
 * Center: The center of the viewport
 * Extents: The distance from the center of the viewport to the edge
 */
struct viewport vp_init(struct vec2 center, struct vec2 extents);
void vp_update_cellsize(struct viewport *vp);
void vp_shift(struct viewport *vp, enum DIR direction, int cells);
void vp_move(struct viewport *vp, struct vec2 new_center);
void vp_zoom(struct viewport *vp, enum ZOOM zoom);
