// Race: nqueens (c)
// Extracted from suite/c/src/algo_dp.c — build and run via suite/run.py

/* Dynamic programming, string and recursion kernels of group `algo` (SPEC.md section 12). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

typedef struct {
    size_t n;
    bool *cols;
    bool *up;
    bool *down;
} Board;

static uint64_t place(Board *b, size_t row) {
    if (row == b->n) {
        return 1;
    }
    uint64_t count = 0;
    for (size_t col = 0; col < b->n; col++) {
        size_t u = row + col, d = row + b->n - 1 - col;
        if (!b->cols[col] && !b->up[u] && !b->down[d]) {
            b->cols[col] = b->up[u] = b->down[d] = true;
            count += place(b, row + 1);
            b->cols[col] = b->up[u] = b->down[d] = false;
        }
    }
    return count;
}

static uint64_t nqueens_run(size_t n) {
    Board b = {n, xcalloc(n, sizeof(bool)), xcalloc(2 * n - 1, sizeof(bool)),
               xcalloc(2 * n - 1, sizeof(bool))};
    uint64_t count = place(&b, 0);
    free(b.cols);
    free(b.up);
    free(b.down);
    return count;
}

static bool nqueens(const Args *args, Result *out) {
    uint64_t v = 0;
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        v = nqueens_run(args->size);
        sink = v;
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu64, v);
    return true;
}
