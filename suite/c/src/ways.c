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

static bool int_random(const Args *args, Result *out) {
    return qsort_ints(args, out, ORDER_RANDOM);
}

static bool int_sorted(const Args *args, Result *out) {
    return qsort_ints(args, out, ORDER_SORTED);
}

static bool int_reversed(const Args *args, Result *out) {
    return qsort_ints(args, out, ORDER_REVERSED);
}

static bool str_random(const Args *args, Result *out) {
    return qsort_strings(args, out, ORDER_RANDOM);
}

static bool str_sorted(const Args *args, Result *out) {
    return qsort_strings(args, out, ORDER_SORTED);
}

static bool str_reversed(const Args *args, Result *out) {
    return qsort_strings(args, out, ORDER_REVERSED);
}

const Kernel WAYS_KERNELS[] = {
    {"bsort-int.c.qsort", int_random},
    {"bsort-int-sorted.c.qsort", int_sorted},
    {"bsort-int-reversed.c.qsort", int_reversed},
    {"bsort-str.c.qsort", str_random},
    {"bsort-str-sorted.c.qsort", str_sorted},
    {"bsort-str-reversed.c.qsort", str_reversed},
    {NULL, NULL},
};
