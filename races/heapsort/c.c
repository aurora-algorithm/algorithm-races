// Race: heapsort (c)
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

static void sift_down(int64_t *a, size_t root, size_t end) {
    while (2 * root + 1 < end) {
        size_t child = 2 * root + 1;
        if (child + 1 < end && a[child] < a[child + 1]) {
            child++;
        }
        if (a[root] < a[child]) {
            swap(a, root, child);
            root = child;
        } else {
            return;
        }
    }
}

static void heap_sort(int64_t *a, size_t n, size_t threads) {
    (void)threads;
    for (size_t start = n / 2; start-- > 0;) {
        sift_down(a, start, n);
    }
    for (size_t end = n; end-- > 1;) {
        swap(a, 0, end);
        sift_down(a, 0, end);
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

static bool sort_heap(const Args *args, Result *out) {
    return run_sort(args, out, heap_sort);
}
