// Race: life (c)
// Extracted from suite/c/src/puzzle.c — build and run via suite/run.py

/* Unusual algorithm kernels (SPEC.md section 13, group `puzzle`): one clever trick each. The string
 * kernels are in puzzle_text.c, sudoku in sudoku.c. */
#include "puzzle.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

#define LIFE_SIDE 512

static unsigned char *life_run(const unsigned char *grid, size_t generations) {
    const size_t w = LIFE_SIDE;
    unsigned char *cur = xmalloc(w * w);
    unsigned char *next = xmalloc(w * w);
    memcpy(cur, grid, w * w);
    for (size_t g = 0; g < generations; g++) {
        for (size_t y = 0; y < w; y++) {
            size_t up = (y + w - 1) % w * w;
            size_t row = y * w;
            size_t down = (y + 1) % w * w;
            for (size_t x = 0; x < w; x++) {
                size_t left = (x + w - 1) % w;
                size_t right = (x + 1) % w;
                unsigned n = cur[up + left] + cur[up + x] + cur[up + right] + cur[row + left] +
                             cur[row + right] + cur[down + left] + cur[down + x] +
                             cur[down + right];
                next[row + x] = (unsigned char)(n == 3 || (n == 2 && cur[row + x] == 1));
            }
        }
        unsigned char *t = cur;
        cur = next;
        next = t;
    }
    free(next);
    return cur;
}

static bool life(const Args *args, Result *out) {
    const size_t cells = LIFE_SIDE * LIFE_SIDE;
    unsigned char *grid = xmalloc(cells);
    Rng rng = rng_new();
    for (size_t i = 0; i < cells; i++) {
        grid[i] = (unsigned char)(rng_next(&rng) % 4 == 0);
    }
    unsigned char *result = NULL;
    for (size_t r = 0; r < args->repeats; r++) {
        free(result);
        double start = now_ms();
        result = life_run(grid, args->size);
        out->times[r] = now_ms() - start;
    }
    size_t alive = 0;
    for (size_t i = 0; i < cells; i++) {
        alive += result[i];
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu:%" PRIu32, args->size, alive,
             fnv1a(result, cells));
    free(result);
    free(grid);
    return true;
}
