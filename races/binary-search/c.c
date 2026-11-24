// Race: binary-search (c)
// Extracted from suite/c/src/algo_dp.c — build and run via suite/run.py

/* Dynamic programming, string and recursion kernels of group `algo` (SPEC.md section 12). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

typedef struct {
    uint64_t hits, total;
} SearchResult;

static SearchResult binary_search_run(const int64_t *a, const int64_t *queries, size_t n) {
    uint64_t hits = 0, total = 0;
    for (size_t i = 0; i < n; i++) {
        int64_t q = queries[i];
        size_t lo = 0, hi = n;
        while (lo < hi) {
            size_t mid = (lo + hi) / 2;
            if (a[mid] < q) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        if (lo < n && a[lo] == q) {
            hits++;
        }
        total += lo;
    }
    return (SearchResult){hits, total};
}

static bool binary_search(const Args *args, Result *out) {
    size_t n = args->size;
    int64_t *a = xmalloc(n * sizeof *a);
    int64_t *queries = xmalloc(n * sizeof *queries);
    Rng rng = rng_new();
    int64_t v = 0;
    for (size_t i = 0; i < n; i++) {
        v += 1 + rng_next(&rng) % 4;
        a[i] = v;
    }
    for (size_t i = 0; i < n; i++) {
        queries[i] = (int64_t)rng_next(&rng) % (v + 1);
    }
    SearchResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = binary_search_run(a, queries, n);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu64 ":%" PRIu64, res.hits, res.total);
    free(a);
    free(queries);
    return true;
}
