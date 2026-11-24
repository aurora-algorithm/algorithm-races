// Race: sieve (c)
// Extracted from suite/c/src/loop.c — build and run via suite/run.py

/* Loop kernels (SPEC.md section 5): sieve and matmul. */
#include <stdio.h>
#include <stdlib.h>
#include "kernels.h"

static size_t sieve_count(size_t n) {
    bool *composite = xcalloc(n + 1, sizeof *composite);
    for (size_t i = 2; i * i <= n; i++) {
        if (!composite[i]) {
            for (size_t j = i * i; j <= n; j += i) {
                composite[j] = true;
            }
        }
    }
    size_t primes = 0;
    for (size_t i = 2; i <= n; i++) {
        primes += !composite[i];
    }
    free(composite);
    return primes;
}

static bool sieve(const Args *args, Result *out) {
    size_t primes = 0;
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        primes = sieve_count(args->size);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu", primes);
    return true;
}
