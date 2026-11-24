#ifndef KERNELS_PUZZLE_H
#define KERNELS_PUZZLE_H

#include "common.h"

#define ONE_MIB ((size_t)1 << 20)

/* MurmurHash3's 32-bit finalizer: the one hash of the probabilistic kernels in every language. */
static inline uint32_t fmix32(uint32_t h) {
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

/* Fisher-Yates: for i = len-1 down to 1, swap a[i] with a[next() % (i+1)]. */
static inline void shuffle(Rng *rng, size_t *a, size_t len) {
    for (size_t i = len; i-- > 1;) {
        size_t j = rng_next(rng) % (i + 1);
        size_t t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
}

/* A binary min-heap of uint64 keys (C has no priority queue in its standard library). */
typedef struct {
    uint64_t *keys;
    size_t len, cap;
} U64Heap;

void u64heap_push(U64Heap *h, uint64_t key);
uint64_t u64heap_pop(U64Heap *h);

#endif
