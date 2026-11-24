// Race: bloom (c)
// Extracted from suite/c/src/puzzle.c — build and run via suite/run.py

/* Unusual algorithm kernels (SPEC.md section 13, group `puzzle`): one clever trick each. The string
 * kernels are in puzzle_text.c, sudoku in sudoku.c. */
#include "puzzle.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

#define BLOOM_BITS_PER_KEY 10

#define BLOOM_HASHES 7u

#define GOLDEN 0x9e3779b9u

typedef struct {
    size_t set_bits, false_positives;
} BloomResult;

static BloomResult bloom_run(const int64_t *keys, const int64_t *lookups, size_t n) {
    uint32_t m = (uint32_t)(BLOOM_BITS_PER_KEY * n);
    unsigned char *bits = xcalloc((m + 7) / 8, 1);
    for (size_t k = 0; k < n; k++) {
        uint32_t h1 = fmix32((uint32_t)keys[k]);
        uint32_t h2 = fmix32((uint32_t)keys[k] ^ GOLDEN) | 1;
        for (uint32_t i = 0; i < BLOOM_HASHES; i++) {
            uint32_t b = (h1 + i * h2) % m;
            bits[b >> 3] |= (unsigned char)(1u << (b & 7));
        }
    }
    size_t false_positives = 0;
    for (size_t k = 0; k < n; k++) {
        uint32_t h1 = fmix32((uint32_t)lookups[k]);
        uint32_t h2 = fmix32((uint32_t)lookups[k] ^ GOLDEN) | 1;
        bool all = true;
        for (uint32_t i = 0; i < BLOOM_HASHES && all; i++) {
            uint32_t b = (h1 + i * h2) % m;
            all = (bits[b >> 3] & (1u << (b & 7))) != 0;
        }
        false_positives += all;
    }
    size_t set_bits = 0;
    for (size_t i = 0; i < (m + 7) / 8; i++) {
        for (unsigned v = bits[i]; v != 0; v &= v - 1) {
            set_bits++;
        }
    }
    free(bits);
    return (BloomResult){set_bits, false_positives};
}

static bool bloom(const Args *args, Result *out) {
    size_t n = args->size;
    int64_t *values = ints(2 * n);
    BloomResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = bloom_run(values, values + n, n);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu:%zu", n, res.set_bits, res.false_positives);
    free(values);
    return true;
}
