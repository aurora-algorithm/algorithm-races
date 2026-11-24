// Race: bsort-str-reversed.rust.sort (c)
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

static bool qsort_strings(const Args *args, Result *out, Order order) {
    size_t n = args->size;
    char *block;
    char **input = strings(n, &block);
    arrange(input, n, sizeof *input, compare_str, order);
    char **s = xmalloc(n * sizeof *s);
    for (size_t r = 0; r < args->repeats; r++) {
        memcpy(s, input, n * sizeof *s);
        double start = now_ms();
        qsort(s, n, sizeof *s, compare_str);
        out->times[r] = now_ms() - start;
    }
    Bytes text = {0};
    for (size_t i = 0; i < n; i++) {
        bytes_append(&text, s[i], strlen(s[i]));
        bytes_append(&text, "\n", 1);
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu32, fnv1a(text.data, text.len));
    free(text.data);
    free(s);
    free(input);
    free(block);
    return true;
}

static bool str_reversed(const Args *args, Result *out) {
    return qsort_strings(args, out, ORDER_REVERSED);
}
