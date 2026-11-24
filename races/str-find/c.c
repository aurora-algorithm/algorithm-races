// Race: str-find (c)
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

static Matches str_find_run(const char *t, size_t n, const char *p) {
    (void)n;
    Matches found = {0, -1, -1};
    for (const char *at = strstr(t, p); at != NULL; at = strstr(at + 1, p)) {
        matches_add(&found, (size_t)(at - t));
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

static bool str_find(const Args *args, Result *out) {
    return run_search(args, out, dna_text, str_find_run);
}
