// Race: knapsack (c)
// Extracted from suite/c/src/algo_dp.c — build and run via suite/run.py

/* Dynamic programming, string and recursion kernels of group `algo` (SPEC.md section 12). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

#define KNAPSACK_ITEMS 1000

typedef struct {
    size_t weight;
    uint64_t value;
} Item;

/* The best value for every capacity 0..capacity (one row). */
static uint64_t *knapsack_run(size_t capacity, const Item *items) {
    uint64_t *best = xcalloc(capacity + 1, sizeof *best);
    for (size_t i = 0; i < KNAPSACK_ITEMS; i++) {
        size_t w = items[i].weight;
        for (size_t c = capacity; c >= w; c--) {
            if (best[c - w] + items[i].value > best[c]) {
                best[c] = best[c - w] + items[i].value;
            }
        }
    }
    return best;
}

static bool knapsack(const Args *args, Result *out) {
    Item items[KNAPSACK_ITEMS];
    Rng rng = rng_new();
    for (int i = 0; i < KNAPSACK_ITEMS; i++) {
        items[i].weight = 1 + rng_next(&rng) % 1000;
        items[i].value = 1 + rng_next(&rng) % 1000;
    }
    uint64_t *best = NULL;
    for (size_t r = 0; r < args->repeats; r++) {
        free(best);
        double start = now_ms();
        best = knapsack_run(args->size, items);
        out->times[r] = now_ms() - start;
    }
    uint32_t h = 0;
    for (size_t c = 0; c <= args->size; c++) {
        h = h31_step(h, (int64_t)best[c]);
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu64 ":%" PRIu32, best[args->size], h);
    free(best);
    return true;
}
