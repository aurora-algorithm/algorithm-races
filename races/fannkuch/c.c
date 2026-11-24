// Race: fannkuch (c)
// Extracted from suite/c/src/bg.c — build and run via suite/run.py

/* Benchmarks Game kernels, single-threaded (SPEC.md section 4). Floating-point code keeps the
 * operation order of the spec so every language prints the same digits. */
#include "bg.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

/* The Benchmarks Game's sequential algorithm; returns "{checksum}:{maxflips}". */
static void fannkuch_run(size_t n, char *checksum_out) {
    size_t *perm1 = xmalloc(n * sizeof *perm1);
    size_t *perm = xmalloc(n * sizeof *perm);
    size_t *count = xcalloc(n, sizeof *count);
    for (size_t i = 0; i < n; i++) {
        perm1[i] = i;
    }
    int64_t max_flips = 0, checksum = 0;
    uint64_t perm_count = 0;
    size_t r = n;
    for (;;) {
        for (; r != 1; r--) {
            count[r - 1] = r;
        }
        memcpy(perm, perm1, n * sizeof *perm);
        int64_t flips = 0;
        for (size_t k = perm[0]; k != 0; k = perm[0]) {
            for (size_t i = 0, j = k; i < j; i++, j--) {
                size_t t = perm[i];
                perm[i] = perm[j];
                perm[j] = t;
            }
            flips++;
        }
        if (flips > max_flips) {
            max_flips = flips;
        }
        checksum += perm_count % 2 == 0 ? flips : -flips;
        for (;;) {
            if (r == n) {
                snprintf(checksum_out, CHECKSUM_MAX, "%" PRId64 ":%" PRId64, checksum, max_flips);
                free(perm1);
                free(perm);
                free(count);
                return;
            }
            size_t first = perm1[0];
            memmove(perm1, perm1 + 1, r * sizeof *perm1);
            perm1[r] = first;
            if (--count[r] > 0) {
                break;
            }
            r++;
        }
        perm_count++;
    }
}

static bool fannkuch(const Args *args, Result *out) {
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        fannkuch_run(args->size, out->checksum);
        out->times[r] = now_ms() - start;
    }
    return true;
}
