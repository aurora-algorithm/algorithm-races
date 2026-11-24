// Race: radixsort (c)
// Extracted from suite/c/src/sorts.c — build and run via suite/run.py

/* Sorting kernels, written the same way in every language (SPEC.md section 3). */
#include "sorts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

/* LSD radix sort, 8 bits per pass, for values below 2^32; after 4 passes the result is back in a.
 */
static void radix_sort(int64_t *a, size_t n, size_t threads) {
    (void)threads;
    int64_t *buf = xmalloc(n * sizeof *buf);
    int64_t *from = a;
    int64_t *to = buf;
    for (int shift = 0; shift < 32; shift += 8) {
        size_t count[256] = {0};
        for (size_t i = 0; i < n; i++) {
            count[(from[i] >> shift) & 255]++;
        }
        size_t pos[256];
        size_t total = 0;
        for (int d = 0; d < 256; d++) {
            pos[d] = total;
            total += count[d];
        }
        for (size_t i = 0; i < n; i++) {
            to[pos[(from[i] >> shift) & 255]++] = from[i];
        }
        int64_t *t = from;
        from = to;
        to = t;
    }
    free(buf);
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

static bool sort_radix(const Args *args, Result *out) {
    return run_sort(args, out, radix_sort);
}
