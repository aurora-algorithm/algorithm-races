// Race: edit-distance (c)
// Extracted from suite/c/src/algo_dp.c — build and run via suite/run.py

/* Dynamic programming, string and recursion kernels of group `algo` (SPEC.md section 12). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

static const char ACGT[4] = {'a', 'c', 'g', 't'};

/* `count` strings of `length` letters "acgt"[next() % 4] from one generator, each NUL-terminated.
 */
static char *dna(size_t length, size_t count) {
    Rng rng = rng_new();
    char *s = xmalloc(count * (length + 1));
    for (size_t c = 0; c < count; c++) {
        char *p = s + c * (length + 1);
        for (size_t i = 0; i < length; i++) {
            p[i] = ACGT[rng_next(&rng) % 4];
        }
        p[length] = '\0';
    }
    return s;
}

static size_t min3(size_t a, size_t b, size_t c) {
    size_t m = a < b ? a : b;
    return m < c ? m : c;
}

/* edit_distance_run with the left neighbour (cur[j] + 1) as the last argument of min3; runtime
 * c-clang-edleft. */
static size_t edit_distance_left_last(const char *a, const char *b, size_t n) {
    size_t *prev = xmalloc((n + 1) * sizeof *prev);
    size_t *cur = xmalloc((n + 1) * sizeof *cur);
    for (size_t j = 0; j <= n; j++) {
        prev[j] = j;
    }
    for (size_t i = 0; i < n; i++) {
        cur[0] = i + 1;
        for (size_t j = 0; j < n; j++) {
            size_t cost = a[i] != b[j];
            cur[j + 1] = min3(prev[j + 1] + 1, prev[j] + cost, cur[j] + 1);
        }
        size_t *t = prev;
        prev = cur;
        cur = t;
    }
    size_t d = prev[n];
    free(prev);
    free(cur);
    return d;
}

static size_t edit_distance_run(const char *a, const char *b, size_t n) {
    size_t *prev = xmalloc((n + 1) * sizeof *prev);
    size_t *cur = xmalloc((n + 1) * sizeof *cur);
    for (size_t j = 0; j <= n; j++) {
        prev[j] = j;
    }
    for (size_t i = 0; i < n; i++) {
        cur[0] = i + 1;
        for (size_t j = 0; j < n; j++) {
            size_t cost = a[i] != b[j];
            cur[j + 1] = min3(prev[j + 1] + 1, cur[j] + 1, prev[j] + cost);
        }
        size_t *t = prev;
        prev = cur;
        cur = t;
    }
    size_t d = prev[n];
    free(prev);
    free(cur);
    return d;
}

static bool edit_distance(const Args *args, Result *out) {
    size_t n = args->size;
    char *ab = dna(n, 2);
    const char *order = getenv("KERNELS_ED_MIN");
    size_t (*run)(const char *, const char *, size_t) =
        order != NULL && strcmp(order, "left-last") == 0 ? edit_distance_left_last
                                                         : edit_distance_run;
    size_t d = 0;
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        d = run(ab, ab + n + 1, n);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu", d);
    free(ab);
    return true;
}
