// Race: quicksort (c)
// Extracted from suite/c/src/sorts.c — build and run via suite/run.py

/* Sorting kernels, written the same way in every language (SPEC.md section 3). */
#include "sorts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

static void swap(int64_t *a, size_t i, size_t j) {
    int64_t t = a[i];
    a[i] = a[j];
    a[j] = t;
}

/* Hoare partition with the middle element as pivot; inclusive bounds.
 * Recurses into the smaller side and loops on the larger one. */
static void quicksort_range(int64_t *a, size_t lo, size_t hi) {
    while (lo < hi) {
        int64_t pivot = a[lo + (hi - lo) / 2];
        size_t i = lo;
        size_t j = hi;
        for (;;) {
            while (a[i] < pivot) {
                i++;
            }
            while (a[j] > pivot) {
                j--;
            }
            if (i >= j) {
                break;
            }
            swap(a, i, j);
            i++;
            j--;
        }
        if (j - lo < hi - j) {
            quicksort_range(a, lo, j);
            lo = j + 1;
        } else {
            quicksort_range(a, j + 1, hi);
            hi = j;
        }
    }
}

static void quick_sort(int64_t *a, size_t n, size_t threads) {
    (void)threads;
    if (n > 1) {
        quicksort_range(a, 0, n - 1);
    }
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

static bool sort_quick(const Args *args, Result *out) {
    return run_sort(args, out, quick_sort);
}
