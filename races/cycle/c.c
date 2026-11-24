// Race: cycle (c)
// Extracted from suite/c/src/puzzle.c — build and run via suite/run.py

/* Unusual algorithm kernels (SPEC.md section 13, group `puzzle`): one clever trick each. The string
 * kernels are in puzzle_text.c, sudoku in sudoku.c. */
#include "puzzle.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

#define CYCLE_MOD 67108859u

#define CYCLE_C 12345u

static uint64_t step(uint64_t x) {
    return (x * x + CYCLE_C) % CYCLE_MOD;
}

typedef struct {
    uint64_t total_mu, total_lam, longest;
} CycleResult;

static CycleResult cycle_run(const uint64_t *starts, size_t n) {
    uint64_t total_mu = 0, total_lam = 0, longest = 0;
    for (size_t i = 0; i < n; i++) {
        uint64_t x0 = starts[i];
        uint64_t tortoise = step(x0);
        uint64_t hare = step(step(x0));
        while (tortoise != hare) {
            tortoise = step(tortoise);
            hare = step(step(hare));
        }
        uint64_t mu = 0;
        tortoise = x0;
        while (tortoise != hare) {
            tortoise = step(tortoise);
            hare = step(hare);
            mu++;
        }
        uint64_t lam = 1;
        hare = step(tortoise);
        while (tortoise != hare) {
            hare = step(hare);
            lam++;
        }
        total_mu += mu;
        total_lam += lam;
        longest = mu + lam > longest ? mu + lam : longest;
    }
    return (CycleResult){total_mu, total_lam, longest};
}

static bool cycle(const Args *args, Result *out) {
    size_t n = args->size;
    uint64_t *starts = xmalloc(n * sizeof *starts);
    Rng rng = rng_new();
    for (size_t i = 0; i < n; i++) {
        starts[i] = rng_next(&rng) % CYCLE_MOD;
    }
    CycleResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = cycle_run(starts, n);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%" PRIu64 ":%" PRIu64 ":%" PRIu64, n, res.total_mu,
             res.total_lam, res.longest);
    free(starts);
    return true;
}
