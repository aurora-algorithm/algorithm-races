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

static double *matmul_run(const double *a, const double *b, size_t n) {
    double *c = xmalloc(n * n * sizeof *c);
    for (size_t i = 0; i < n; i++) {
        size_t row = i * n;
        for (size_t j = 0; j < n; j++) {
            double s = 0.0;
            for (size_t k = 0; k < n; k++) {
                s += a[row + k] * b[k * n + j];
            }
            c[row + j] = s;
        }
    }
    return c;
}

static bool matmul(const Args *args, Result *out) {
    size_t n = args->size;
    double *a = xmalloc(n * n * sizeof *a);
    double *b = xmalloc(n * n * sizeof *b);
    Rng rng = rng_new();
    for (size_t i = 0; i < n * n; i++) {
        a[i] = (double)(rng_next(&rng) % 10);
    }
    for (size_t i = 0; i < n * n; i++) {
        b[i] = (double)(rng_next(&rng) % 10);
    }
    double *c = NULL;
    for (size_t r = 0; r < args->repeats; r++) {
        free(c);
        double start = now_ms();
        c = matmul_run(a, b, n);
        out->times[r] = now_ms() - start;
    }
    int64_t total = 0;
    uint32_t h = 0;
    for (size_t i = 0; i < n * n; i++) {
        total += (int64_t)c[i];
        h = h31_step(h, (int64_t)c[i]);
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRId64 ":%" PRIu32, total, h);
    free(a);
    free(b);
    free(c);
    return true;
}

const Kernel LOOP_KERNELS[] = {{"sieve", sieve}, {"matmul", matmul}, {NULL, NULL}};
