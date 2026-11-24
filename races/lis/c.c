// Race: lis (c)
// Extracted from suite/c/src/algo_dp.c — build and run via suite/run.py

/* Dynamic programming, string and recursion kernels of group `algo` (SPEC.md section 12). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

/* Patience sorting; C has no lower-bound search (bsearch finds only an equal element). */
static int64_t *lis_run(const int64_t *values, size_t n, size_t *tails_len) {
    size_t len = 0, cap = 16;
    int64_t *tails = xmalloc(cap * sizeof *tails);
    for (size_t i = 0; i < n; i++) {
        int64_t x = values[i];
        size_t lo = 0, hi = len;
        while (lo < hi) {
            size_t mid = lo + (hi - lo) / 2;
            if (tails[mid] < x) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        if (lo == len) {
            if (len == cap) {
                cap *= 2;
                tails = xrealloc(tails, cap * sizeof *tails);
            }
            tails[len++] = x;
        } else {
            tails[lo] = x;
        }
    }
    *tails_len = len;
    return tails;
}

static bool lis(const Args *args, Result *out) {
    int64_t *values = ints(args->size);
    int64_t *tails = NULL;
    size_t len = 0;
    for (size_t r = 0; r < args->repeats; r++) {
        free(tails);
        double start = now_ms();
        tails = lis_run(values, args->size, &len);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%" PRIu32, len, h31(tails, len));
    free(tails);
    free(values);
    return true;
}
