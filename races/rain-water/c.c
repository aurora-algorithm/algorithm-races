// Race: rain-water (c)
// Extracted from suite/c/src/puzzle.c — build and run via suite/run.py

/* Unusual algorithm kernels (SPEC.md section 13, group `puzzle`): one clever trick each. The string
 * kernels are in puzzle_text.c, sudoku in sudoku.c. */
#include "puzzle.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

void u64heap_push(U64Heap *h, uint64_t key) {
    if (h->len == h->cap) {
        h->cap = h->cap ? h->cap * 2 : 16;
        h->keys = xrealloc(h->keys, h->cap * sizeof *h->keys);
    }
    size_t i = h->len++;
    while (i > 0 && key < h->keys[(i - 1) / 2]) {
        h->keys[i] = h->keys[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    h->keys[i] = key;
}

uint64_t u64heap_pop(U64Heap *h) {
    uint64_t top = h->keys[0];
    uint64_t last = h->keys[--h->len];
    size_t i = 0;
    for (;;) {
        size_t child = 2 * i + 1;
        if (child >= h->len) {
            break;
        }
        if (child + 1 < h->len && h->keys[child + 1] < h->keys[child]) {
            child++;
        }
        if (h->keys[child] >= last) {
            break;
        }
        h->keys[i] = h->keys[child];
        i = child;
    }
    if (h->len > 0) {
        h->keys[i] = last;
    }
    return top;
}

typedef struct {
    uint64_t water, wet;
} RainResult;

static RainResult rain_water_run(size_t side, const uint64_t *heights) {
    size_t n = side * side;
    bool *seen = xcalloc(n, sizeof *seen);
    U64Heap heap = {0};
    for (size_t idx = 0; idx < n; idx++) {
        size_t r = idx / side, c = idx % side;
        if (r == 0 || r == side - 1 || c == 0 || c == side - 1) {
            seen[idx] = true;
            u64heap_push(&heap, heights[idx] * n + idx);
        }
    }
    uint64_t water = 0, wet = 0;
    while (heap.len > 0) {
        uint64_t key = u64heap_pop(&heap);
        uint64_t level = key / n;
        size_t idx = (size_t)(key % n);
        size_t r = idx / side, c = idx % side;
        const struct {
            size_t next;
            bool ok;
        } neighbours[4] = {{idx - side, r > 0},
                           {idx + side, r + 1 < side},
                           {idx - 1, c > 0},
                           {idx + 1, c + 1 < side}};
        for (int k = 0; k < 4; k++) {
            size_t next = neighbours[k].next;
            if (!neighbours[k].ok || seen[next]) {
                continue;
            }
            seen[next] = true;
            uint64_t h = heights[next];
            if (h < level) {
                water += level - h;
                wet++;
                h = level;
            }
            u64heap_push(&heap, h * n + next);
        }
    }
    free(heap.keys);
    free(seen);
    return (RainResult){water, wet};
}

static bool rain_water(const Args *args, Result *out) {
    size_t side = args->size;
    uint64_t *heights = xmalloc(side * side * sizeof *heights);
    Rng rng = rng_new();
    for (size_t i = 0; i < side * side; i++) {
        heights[i] = rng_next(&rng) % 1000;
    }
    RainResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = rain_water_run(side, heights);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%" PRIu64 ":%" PRIu64, side, res.water, res.wet);
    free(heights);
    return true;
}
