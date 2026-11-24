// Race: collatz (c)
// Extracted from suite/c/src/puzzle.c — build and run via suite/run.py

/* Unusual algorithm kernels (SPEC.md section 13, group `puzzle`): one clever trick each. The string
 * kernels are in puzzle_text.c, sudoku in sudoku.c. */
#include "puzzle.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

typedef struct {
    size_t best;
    uint32_t best_len;
    uint64_t total;
} CollatzResult;

static CollatzResult collatz_run(size_t n) {
    uint32_t *cache = xcalloc(n, sizeof *cache);
    cache[1] = 1;
    size_t best = 1;
    uint32_t best_len = 1;
    uint64_t total = 1;
    for (size_t start = 2; start < n; start++) {
        uint64_t x = start;
        uint32_t steps = 0;
        while (x >= start) {
            x = x % 2 == 0 ? x / 2 : 3 * x + 1;
            steps++;
        }
        uint32_t len = steps + cache[x];
        cache[start] = len;
        total += len;
        if (len > best_len) {
            best = start;
            best_len = len;
        }
    }
    free(cache);
    return (CollatzResult){best, best_len, total};
}

static bool collatz(const Args *args, Result *out) {
    CollatzResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = collatz_run(args->size);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu:%" PRIu32 ":%" PRIu64, args->size, res.best,
             res.best_len, res.total);
    return true;
}
