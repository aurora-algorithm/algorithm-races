// Race: bsort-int-sorted.goose.sort (c)
// Extracted from suite/c/src/ways.c — build and run via suite/run.py

/* The builtins group (../../builtins-NOTES.md) for C: the C standard library's one way to sort,
 * qsort, on the three input orders of ints(N) and strings(N). Kernel names are JOB.c.WAY. C has
 * no standard hash map and no string builder, so it has no map or join rows. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"
#include "sorts.h"

typedef enum { ORDER_RANDOM, ORDER_SORTED, ORDER_REVERSED } Order;

/* Puts the input in the job's order before timing: as generated, ascending or descending. */
static void arrange(void *base, size_t n, size_t width, int (*cmp)(const void *, const void *),
                    Order order) {
    if (order == ORDER_RANDOM) {
        return;
    }
    qsort(base, n, width, cmp);
    if (order == ORDER_REVERSED && n > 1) {
        unsigned char *a = base;
        unsigned char tmp[sizeof(int64_t) > sizeof(char *) ? sizeof(int64_t) : sizeof(char *)];
        for (size_t i = 0, j = n - 1; i < j; i++, j--) {
            memcpy(tmp, a + i * width, width);
            memcpy(a + i * width, a + j * width, width);
            memcpy(a + j * width, tmp, width);
        }
    }
}

static bool qsort_ints(const Args *args, Result *out, Order order) {
    size_t n = args->size;
    int64_t *input = ints(n);
    arrange(input, n, sizeof *input, compare_i64, order);
    int64_t *a = xmalloc(n * sizeof *a);
    for (size_t r = 0; r < args->repeats; r++) {
        memcpy(a, input, n * sizeof *a);
        double start = now_ms();
        qsort(a, n, sizeof *a, compare_i64);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu32, h31(a, n));
    free(a);
    free(input);
    return true;
}

static bool int_sorted(const Args *args, Result *out) {
    return qsort_ints(args, out, ORDER_SORTED);
}
