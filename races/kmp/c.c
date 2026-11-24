// Race: kmp (c)
// Extracted from suite/c/src/algo_dp.c — build and run via suite/run.py

/* Dynamic programming, string and recursion kernels of group `algo` (SPEC.md section 12). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

#define PATTERN_LEN 8

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

typedef struct {
    size_t count;
    int64_t first;
    int64_t last;
} Matches;

static void matches_add(Matches *m, size_t pos) {
    if (m->first < 0) {
        m->first = (int64_t)pos;
    }
    m->last = (int64_t)pos;
    m->count++;
}

static Matches kmp_run(const char *t, size_t n, const char *p) {
    size_t fail[PATTERN_LEN] = {0};
    size_t k = 0;
    for (size_t i = 1; i < PATTERN_LEN; i++) {
        while (k > 0 && p[i] != p[k]) {
            k = fail[k - 1];
        }
        if (p[i] == p[k]) {
            k++;
        }
        fail[i] = k;
    }
    Matches found = {0, -1, -1};
    k = 0;
    for (size_t i = 0; i < n; i++) {
        while (k > 0 && t[i] != p[k]) {
            k = fail[k - 1];
        }
        if (t[i] == p[k]) {
            k++;
        }
        if (k == PATTERN_LEN) {
            matches_add(&found, i + 1 - PATTERN_LEN);
            k = fail[k - 1];
        }
    }
    return found;
}

static char *dna_text(size_t n) {
    return dna(n, 1);
}

static bool run_search(const Args *args, Result *out, char *(*build)(size_t),
                       Matches (*search)(const char *, size_t, const char *)) {
    size_t n = args->size;
    char *t = build(n);
    char p[PATTERN_LEN + 1];
    memcpy(p, t + n / 2, PATTERN_LEN);
    p[PATTERN_LEN] = '\0';
    Matches m = {0, -1, -1};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        m = search(t, n, p);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%" PRId64 ":%" PRId64, m.count, m.first, m.last);
    free(t);
    return true;
}

static bool kmp(const Args *args, Result *out) {
    return run_search(args, out, dna_text, kmp_run);
}
