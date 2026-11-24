// Race: sort-int (c)
// Extracted from suite/c/src/sorts.c — build and run via suite/run.py

/* Sorting kernels, written the same way in every language (SPEC.md section 3). */
#include "sorts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

int compare_i64(const void *pa, const void *pb) {
    int64_t a = *(const int64_t *)pa;
    int64_t b = *(const int64_t *)pb;
    return (a > b) - (a < b);
}

static void std_sort(int64_t *a, size_t n, size_t threads) {
    (void)threads;
    qsort(a, n, sizeof *a, compare_i64);
}

bool run_sort(const Args *args, Result *out, SortFn sort) {
    size_t n = args->size;
    int64_t *input = ints(n);
    int64_t *a = xmalloc(n * sizeof *a);
    for (size_t r = 0; r < args->repeats; r++) {
        memcpy(a, input, n * sizeof *a);
        double start = now_ms();
        sort(a, n, args->threads);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu32, h31(a, n));
    free(a);
    free(input);
    return true;
}

static bool sort_int(const Args *args, Result *out) {
    return run_sort(args, out, std_sort);
}
