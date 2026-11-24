// Race: fib (c)
// Extracted from suite/c/src/algo_dp.c — build and run via suite/run.py

/* Dynamic programming, string and recursion kernels of group `algo` (SPEC.md section 12). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

static uint64_t fib_run(unsigned n) {
    return n < 2 ? n : fib_run(n - 1) + fib_run(n - 2);
}

static bool fib(const Args *args, Result *out) {
    if (args->size > 90) {
        fprintf(stderr, "error: --size %zu is too large\n", args->size);
        return false;
    }
    uint64_t v = 0;
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        v = fib_run((unsigned)args->size);
        sink = v;
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu64, v);
    return true;
}
