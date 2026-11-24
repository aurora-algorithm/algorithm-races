// Race: hyperloglog (c)
// Extracted from suite/c/src/puzzle.c — build and run via suite/run.py

/* Unusual algorithm kernels (SPEC.md section 13, group `puzzle`): one clever trick each. The string
 * kernels are in puzzle_text.c, sudoku in sudoku.c. */
#include "puzzle.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

#define HLL_P 14

#define HLL_M (1u << HLL_P)

/* C17 has no count-leading-zeros (C23's stdc_leading_zeros is not in MSVC 14.38), so the compilers'
 * own intrinsic, as Rust's u32::leading_zeros and Go's bits.LeadingZeros32 compile to. w is not 0.
 */
static unsigned leading_zeros(uint32_t w) {
#ifdef _MSC_VER
    unsigned long index;
    _BitScanReverse(&index, w);
    return 31u - (unsigned)index;
#else
    return (unsigned)__builtin_clz(w);
#endif
}

/* Fills the registers and returns the estimate. */
static double hyperloglog_run(size_t n, unsigned char *registers) {
    Rng rng = rng_new();
    uint32_t universe = (uint32_t)(n / 4 > 1 ? n / 4 : 1);
    memset(registers, 0, HLL_M);
    for (size_t i = 0; i < n; i++) {
        uint32_t h = fmix32(rng_next(&rng) % universe);
        uint32_t w = h << HLL_P;
        unsigned rank = w == 0 ? 32 - HLL_P + 1 : leading_zeros(w) + 1;
        uint32_t idx = h >> (32 - HLL_P);
        if (rank > registers[idx]) {
            registers[idx] = (unsigned char)rank;
        }
    }
    double z = 0.0;
    size_t zeros = 0;
    for (uint32_t i = 0; i < HLL_M; i++) {
        z += pow(2.0, -(double)registers[i]);
        zeros += registers[i] == 0;
    }
    double m = (double)HLL_M;
    double estimate = 0.7213 / (1.0 + 1.079 / m) * m * m / z;
    if (estimate <= 2.5 * m && zeros > 0) {
        estimate = m * log(m / (double)zeros);
    }
    return estimate;
}

static bool hyperloglog(const Args *args, Result *out) {
    unsigned char *registers = xmalloc(HLL_M);
    double estimate = 0.0;
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        estimate = hyperloglog_run(args->size, registers);
        out->times[r] = now_ms() - start;
    }
    uint32_t hash = 0;
    for (uint32_t i = 0; i < HLL_M; i++) {
        hash = h31_step(hash, registers[i]);
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%.0f:%" PRIu32, args->size, floor(estimate + 0.5),
             hash);
    free(registers);
    return true;
}
