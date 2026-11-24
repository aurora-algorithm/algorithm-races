// Race: window-max (c)
// Extracted from suite/c/src/puzzle.c — build and run via suite/run.py

/* Unusual algorithm kernels (SPEC.md section 13, group `puzzle`): one clever trick each. The string
 * kernels are in puzzle_text.c, sudoku in sudoku.c. */
#include "puzzle.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

static const size_t WINDOWS[5] = {10, 100, 1000, 10000, 100000};

/* The h31 of each window size's maxima is folded in as they come: keeping every maximum for after
 * the timer would take 5 x N values of memory (see NOTES.md). */
static void window_max_run(const int64_t *values, size_t n, uint32_t hashes[5]) {
    size_t *dq = xmalloc(n * sizeof *dq);
    for (int k = 0; k < 5; k++) {
        size_t w = WINDOWS[k];
        size_t head = 0, tail = 0;
        uint32_t h = 0;
        for (size_t i = 0; i < n; i++) {
            while (tail > head && values[dq[tail - 1]] <= values[i]) {
                tail--;
            }
            dq[tail++] = i;
            if (dq[head] + w <= i) {
                head++;
            }
            if (i + 1 >= w) {
                h = h31_step(h, values[dq[head]]);
            }
        }
        hashes[k] = h;
    }
    free(dq);
}

static bool window_max(const Args *args, Result *out) {
    int64_t *values = ints(args->size);
    uint32_t h[5] = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        window_max_run(values, args->size, h);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX,
             "%zu:%" PRIu32 ":%" PRIu32 ":%" PRIu32 ":%" PRIu32 ":%" PRIu32, args->size, h[0], h[1],
             h[2], h[3], h[4]);
    free(values);
    return true;
}
