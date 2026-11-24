/* Dynamic programming, string and recursion kernels of group `algo` (SPEC.md section 12). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernels.h"

#define KNAPSACK_ITEMS 1000
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

/* The 64 most common English words (SPEC.md section 12, kmp-en). */
static const char *const WORDS[64] = {
    "the",   "be",   "to",   "of",    "and",   "a",      "in",   "that", "have", "i",    "it",
    "for",   "not",  "on",   "with",  "he",    "as",     "you",  "do",   "at",   "this", "but",
    "his",   "by",   "from", "they",  "we",    "say",    "her",  "she",  "or",   "an",   "will",
    "my",    "one",  "all",  "would", "there", "their",  "what", "so",   "up",   "out",  "if",
    "about", "who",  "get",  "which", "go",    "me",     "when", "make", "can",  "like", "time",
    "no",    "just", "him",  "know",  "take",  "people", "into", "year", "your"};

/* n bytes of English-like text, NUL-terminated: words from WORDS separated by " ", ", " or ". ". */
static char *english(size_t n) {
    Rng rng = rng_new();
    char *s = xmalloc(n + 32);
    size_t len = 0;
    while (len < n) {
        uint32_t a = rng_next(&rng) % 64;
        uint32_t b = rng_next(&rng) % 64;
        const char *w = WORDS[a < b ? a : b];
        uint32_t r = rng_next(&rng) % 16;
        const char *sep = r == 0 ? ". " : r == 1 ? ", " : " ";
        size_t wl = strlen(w);
        size_t sl = strlen(sep);
        memcpy(s + len, w, wl);
        memcpy(s + len + wl, sep, sl);
        len += wl + sl;
    }
    s[n] = '\0';
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

typedef struct {
    size_t weight;
    uint64_t value;
} Item;

/* The best value for every capacity 0..capacity (one row). */
static uint64_t *knapsack_run(size_t capacity, const Item *items) {
    uint64_t *best = xcalloc(capacity + 1, sizeof *best);
    for (size_t i = 0; i < KNAPSACK_ITEMS; i++) {
        size_t w = items[i].weight;
        for (size_t c = capacity; c >= w; c--) {
            if (best[c - w] + items[i].value > best[c]) {
                best[c] = best[c - w] + items[i].value;
            }
        }
    }
    return best;
}

static bool knapsack(const Args *args, Result *out) {
    Item items[KNAPSACK_ITEMS];
    Rng rng = rng_new();
    for (int i = 0; i < KNAPSACK_ITEMS; i++) {
        items[i].weight = 1 + rng_next(&rng) % 1000;
        items[i].value = 1 + rng_next(&rng) % 1000;
    }
    uint64_t *best = NULL;
    for (size_t r = 0; r < args->repeats; r++) {
        free(best);
        double start = now_ms();
        best = knapsack_run(args->size, items);
        out->times[r] = now_ms() - start;
    }
    uint32_t h = 0;
    for (size_t c = 0; c <= args->size; c++) {
        h = h31_step(h, (int64_t)best[c]);
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu64 ":%" PRIu32, best[args->size], h);
    free(best);
    return true;
}

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

static bool kmp(const Args *args, Result *out) {
    return run_search(args, out, dna_text, kmp_run);
}

static bool str_find(const Args *args, Result *out) {
    return run_search(args, out, dna_text, str_find_run);
}

static bool kmp_en(const Args *args, Result *out) {
    return run_search(args, out, english, kmp_run);
}

static bool str_find_en(const Args *args, Result *out) {
    return run_search(args, out, english, str_find_run);
}

static uint64_t fib_run(unsigned n) {
    return n < 2 ? n : fib_run(n - 1) + fib_run(n - 2);
}

static bool fib(const Args *args, Result *out) {
    if (args->size > 90) {
        fprintf(stderr, "error: --size %zu is too large\n", args->size);
        return false;
    }
    uint64_t v = 0;
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        v = fib_run((unsigned)args->size);
        sink = v;
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu64, v);
    return true;
}

typedef struct {
    size_t n;
    bool *cols;
    bool *up;
    bool *down;
} Board;

static uint64_t place(Board *b, size_t row) {
    if (row == b->n) {
        return 1;
    }
    uint64_t count = 0;
    for (size_t col = 0; col < b->n; col++) {
        size_t u = row + col, d = row + b->n - 1 - col;
        if (!b->cols[col] && !b->up[u] && !b->down[d]) {
            b->cols[col] = b->up[u] = b->down[d] = true;
            count += place(b, row + 1);
            b->cols[col] = b->up[u] = b->down[d] = false;
        }
    }
    return count;
}

static uint64_t nqueens_run(size_t n) {
    Board b = {n, xcalloc(n, sizeof(bool)), xcalloc(2 * n - 1, sizeof(bool)),
               xcalloc(2 * n - 1, sizeof(bool))};
    uint64_t count = place(&b, 0);
    free(b.cols);
    free(b.up);
    free(b.down);
    return count;
}

static bool nqueens(const Args *args, Result *out) {
    uint64_t v = 0;
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        v = nqueens_run(args->size);
        sink = v;
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu64, v);
    return true;
}

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

const Kernel DP_KERNELS[] = {
    {"edit-distance", edit_distance},
    {"knapsack", knapsack},
    {"lis", lis},
    {"kmp", kmp},
    {"str-find", str_find},
    {"kmp-en", kmp_en},
    {"str-find-en", str_find_en},
    {"fib", fib},
    {"nqueens", nqueens},
    {"binary-search", binary_search},
    {NULL, NULL},
};
