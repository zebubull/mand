#pragma once
#include <math.h>

struct vec2 {
    double x, y;
};

struct complex {
    double real, imag;
};

static inline double complex_mag(struct complex c) {
    return hypot(c.real, c.imag);
}

static inline void complex_square(struct complex *c) {
    double r = c->real;
    double i = c->imag;
    c->real = r * r - i * i;
    c->imag = 2 * r * i;
}

static inline void complex_add(struct complex *a, struct complex *b) {
    a->real += b->real;
    a->imag += b->imag;
}
