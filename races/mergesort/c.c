// Race: mergesort (c)
// Extracted from suite/c/src/sorts.c — build and run via suite/run.py

/* Sorting kernels, written the same way in every language (SPEC.md section 3). */
#include "sorts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

void merge_sort(int64_t *a, int64_t *aux, size_t lo, size_t hi) {
    if (hi - lo < 2) {
        return;
    }
    size_t mid = lo + (hi - lo) / 2;
    merge_sort(a, aux, lo, mid);
    merge_sort(a, aux, mid, hi);
    merge(a, aux, lo, mid, hi);
}

void merge(int64_t *a, int64_t *aux, size_t lo, size_t mid, size_t hi) {
    memcpy(aux + lo, a + lo, (hi - lo) * sizeof *a);
    size_t i = lo;
    size_t j = mid;
    size_t k = lo;
    while (i < mid && j < hi) {
        if (aux[j] < aux[i]) {
            a[k++] = aux[j++];
        } else {
            a[k++] = aux[i++];
        }
    }
    while (i < mid) {
        a[k++] = aux[i++];
    }
    while (j < hi) {
        a[k++] = aux[j++];
    }
}

static void merge_sort_all(int64_t *a, size_t n, size_t threads) {
    (void)threads;
    int64_t *aux = xmalloc(n * sizeof *aux);
    merge_sort(a, aux, 0, n);
    free(aux);
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

static bool sort_merge(const Args *args, Result *out) {
    return run_sort(args, out, merge_sort_all);
}
