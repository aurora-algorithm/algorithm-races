/* The ssearch kernels (../../SPEC.md section 16): lower bound in small sorted arrays, a plain loop
 * against binary search, at every size from 4 to 1024. Kernel names are ssearch.c.WAY. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernels.h"

static const size_t SIZES[] = {4,  6,   8,   12,  16,  24,  32,  48,  64,
                               96, 128, 192, 256, 384, 512, 768, 1024};
#define NSIZES (sizeof SIZES / sizeof SIZES[0])

/* One timed pass of every query through one way; a function per way so the call inlines. */
typedef void (*QueryLoop)(const int64_t *a, size_t n, const int64_t *queries, size_t count,
                          uint64_t *hits, uint64_t *total);

static size_t linear(const int64_t *a, size_t n, int64_t q) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] >= q) {
            return i;
        }
    }
    return n;
}

static size_t linear_count(const int64_t *a, size_t n, int64_t q) {
    size_t count = 0;
    for (size_t i = 0; i < n; i++) {
        count += a[i] < q;
    }
    return count;
}

static size_t binary(const int64_t *a, size_t n, int64_t q) {
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (a[mid] < q) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

static size_t branchless(const int64_t *a, size_t n, int64_t q) {
    const int64_t *base = a;
    size_t len = n;
    while (len > 1) {
        size_t half = len / 2;
        base = base[half - 1] < q ? base + half : base;
        len -= half;
    }
    return (size_t)(base - a) + (len == 1 && *base < q);
}

#define QUERY_LOOP(name, find)                                                                     \
    static void name(const int64_t *a, size_t n, const int64_t *queries, size_t count,             \
                     uint64_t *hits, uint64_t *total) {                                            \
        uint64_t h = 0, t = 0;                                                                     \
        for (size_t i = 0; i < count; i++) {                                                       \
            size_t lo = find(a, n, queries[i]);                                                    \
            h += lo < n && a[lo] == queries[i];                                                    \
            t += lo;                                                                               \
        }                                                                                          \
        *hits = h;                                                                                 \
        *total = t;                                                                                \
    }

QUERY_LOOP(loop_linear, linear)
QUERY_LOOP(loop_linear_count, linear_count)
QUERY_LOOP(loop_binary, binary)
QUERY_LOOP(loop_branchless, branchless)

static int compare_double(const void *x, const void *y) {
    double a = *(const double *)x, b = *(const double *)y;
    return (a > b) - (a < b);
}

static bool run_sweep(const Args *args, Result *out, QueryLoop query_loop) {
    size_t q_count = args->size, repeats = args->repeats;
    double *ns = xcalloc(NSIZES * repeats, sizeof *ns);
    uint64_t hits = 0, total = 0;
    for (size_t r = 0; r < repeats; r++) {
        out->times[r] = 0;
    }
    for (size_t s = 0; s < NSIZES; s++) {
        size_t n = SIZES[s];
        int64_t *a = xmalloc(n * sizeof *a);
        int64_t *queries = xmalloc(q_count * sizeof *queries);
        Rng rng = rng_new();
        int64_t v = 0;
        for (size_t i = 0; i < n; i++) {
            v += 1 + rng_next(&rng) % 4;
            a[i] = v;
        }
        for (size_t i = 0; i < q_count; i++) {
            queries[i] = (int64_t)(rng_next(&rng) % (uint32_t)(v + 1));
        }
        uint64_t h = 0, t = 0;
        for (size_t r = 0; r < repeats; r++) {
            double start = now_ms();
            query_loop(a, n, queries, q_count, &h, &t);
            double ms = now_ms() - start;
            out->times[r] += ms;
            ns[s * repeats + r] = ms * 1e6 / (double)q_count;
        }
        sink = h + t;
        hits += h;
        total += t;
        free(a);
        free(queries);
    }
    size_t len = (size_t)snprintf(out->extra, EXTRA_MAX, "{");
    for (size_t s = 0; s < NSIZES; s++) {
        double *row = ns + s * repeats;
        qsort(row, repeats, sizeof *row, compare_double);
        double median =
            repeats % 2 ? row[repeats / 2] : (row[repeats / 2 - 1] + row[repeats / 2]) / 2;
        len += (size_t)snprintf(out->extra + len, EXTRA_MAX - len, "%s\"ns_%zu\":%.3f",
                                s ? "," : "", SIZES[s], median);
    }
    snprintf(out->extra + len, EXTRA_MAX - len, "}");
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu64 ":%" PRIu64, hits, total);
    free(ns);
    return true;
}

static bool ssearch_linear(const Args *args, Result *out) {
    return run_sweep(args, out, loop_linear);
}

static bool ssearch_linear_count(const Args *args, Result *out) {
    return run_sweep(args, out, loop_linear_count);
}

static bool ssearch_binary(const Args *args, Result *out) {
    return run_sweep(args, out, loop_binary);
}

static bool ssearch_branchless(const Args *args, Result *out) {
    return run_sweep(args, out, loop_branchless);
}

const Kernel SSEARCH_KERNELS[] = {
    {"ssearch.c.linear", ssearch_linear},
    {"ssearch.c.linear-count", ssearch_linear_count},
    {"ssearch.c.binary", ssearch_binary},
    {"ssearch.c.branchless", ssearch_branchless},
    {NULL, NULL},
};
