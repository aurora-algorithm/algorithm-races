/* The builtins group's many-words jobs (../../builtins-NOTES.md, "Many words"): find every place
 * any of K words occurs in a log. C has no Aho-Corasick or regex in its standard library, so the
 * ways are strstr in a loop, one PCRE2 alternation (the regex library of regex-log) and a
 * hand-written Aho-Corasick. Every way prints "{K}:{matches}:{sum of match start offsets}"; the
 * words are 8-digit ids hidden inside longer digit runs (session tokens). They are never a suffix
 * of one another, and no two matches overlap in these logs. */
#define PCRE2_CODE_UNIT_WIDTH 8

#include <pcre2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernels.h"

#define POOL 10000
#define WORD_LEN 8

static const char *const METHODS[] = {"GET", "POST", "PUT", "DELETE"};
static const unsigned STATUSES[] = {200, 201, 204, 301, 304, 400, 401, 403, 404, 500, 502, 503};

typedef struct {
    uint64_t matches;
    uint64_t offsets;
} Found;

typedef struct {
    char *text;
    size_t len;
    char (*words)[WORD_LEN + 1];
    size_t k;
} Input;

/* 0 to 6 random digits into out (7 bytes): the length first, then each digit. */
static void random_digits(Rng *rng, char *out) {
    uint32_t n = rng_next(rng) % 7;
    for (uint32_t i = 0; i < n; i++) {
        out[i] = (char)('0' + rng_next(rng) % 10);
    }
    out[n] = '\0';
}

static void words_input(size_t lines, size_t k, Input *in) {
    Rng rng = rng_new();
    char (*pool)[WORD_LEN + 1] = xmalloc(POOL * sizeof *pool);
    for (size_t i = 0; i < POOL; i++) {
        snprintf(pool[i], sizeof pool[i], "%u", 10000000u + rng_next(&rng) % 90000000u);
    }
    Bytes text = {0};
    uint32_t *users = xmalloc((lines > 0 ? lines : 1) * sizeof *users);
    for (size_t i = 0; i < lines; i++) {
        const char *method = METHODS[rng_next(&rng) % 4];
        unsigned version = 1 + rng_next(&rng) % 3;
        users[i] = rng_next(&rng) % POOL;
        const char *user = pool[users[i]];
        char before[7];
        char after[7];
        random_digits(&rng, before);
        random_digits(&rng, after);
        unsigned status = STATUSES[rng_next(&rng) % 12];
        unsigned ms = rng_next(&rng) % 1000;
        bytes_printf(&text, "%s /api/v%u/cart?sid=%s%s%s %u %ums\n", method, version, before, user,
                     after, status, ms);
    }
    bytes_push(&text, '\0');
    in->text = (char *)text.data;
    in->len = text.len - 1;
    in->words = xmalloc(k * sizeof *in->words);
    in->k = 0;
    /* The words: the ids of the log's lines in line order, repeats skipped, the first k. */
    for (size_t i = 0; i < lines && in->k < k; i++) {
        const char *id = pool[users[i]];
        bool seen = false;
        for (size_t j = 0; j < in->k && !seen; j++) {
            seen = strcmp(in->words[j], id) == 0;
        }
        if (!seen) {
            memcpy(in->words[in->k++], id, WORD_LEN + 1);
        }
    }
    free(users);
    free(pool);
}

static bool strstr_loop(const Input *in, Found *f) {
    for (size_t w = 0; w < in->k; w++) {
        for (const char *at = strstr(in->text, in->words[w]); at != NULL;
             at = strstr(at + 1, in->words[w])) {
            f->matches++;
            f->offsets += (uint64_t)(at - in->text);
        }
    }
    return true;
}

static bool pcre2_alternation(const Input *in, Found *f) {
    Bytes pattern = {0};
    for (size_t w = 0; w < in->k; w++) {
        if (w > 0) {
            bytes_push(&pattern, '|');
        }
        bytes_append(&pattern, in->words[w], WORD_LEN);
    }
    int error;
    PCRE2_SIZE error_offset;
    pcre2_code *re = pcre2_compile(pattern.data, pattern.len, 0, &error, &error_offset, NULL);
    free(pattern.data);
    if (re == NULL) {
        PCRE2_UCHAR message[256];
        pcre2_get_error_message(error, message, sizeof message);
        fprintf(stderr, "error: pattern at %zu: %s\n", (size_t)error_offset, (char *)message);
        return false;
    }
    pcre2_match_data *md = pcre2_match_data_create_from_pattern(re, NULL);
    PCRE2_SIZE offset = 0;
    while (pcre2_match(re, (PCRE2_SPTR)in->text, in->len, offset, 0, md, NULL) > 0) {
        PCRE2_SIZE *ov = pcre2_get_ovector_pointer(md);
        f->matches++;
        f->offsets += ov[0];
        offset = ov[1];
    }
    pcre2_match_data_free(md);
    pcre2_code_free(re);
    return true;
}

/* A dense table of 256 columns per state with the failure links folded in (a DFA). out[s] is
 * the length of the word that ends in state s, 0 when none does. */
static bool hand_aho_corasick(const Input *in, Found *f) {
    size_t cap = in->k * WORD_LEN + 1;
    int32_t *next = xmalloc(cap * 256 * sizeof *next);
    int32_t *fail = xcalloc(cap, sizeof *fail);
    int32_t *out = xcalloc(cap, sizeof *out);
    int32_t *queue = xmalloc(cap * sizeof *queue);
    memset(next, -1, 256 * sizeof *next);
    int32_t states = 1;
    for (size_t w = 0; w < in->k; w++) {
        int32_t s = 0;
        for (const unsigned char *c = (const unsigned char *)in->words[w]; *c; c++) {
            if (next[s * 256 + *c] < 0) {
                memset(next + (size_t)states * 256, -1, 256 * sizeof *next);
                next[s * 256 + *c] = states++;
            }
            s = next[s * 256 + *c];
        }
        out[s] = WORD_LEN;
    }
    size_t head = 0, tail = 0;
    for (int c = 0; c < 256; c++) {
        int32_t t = next[c];
        if (t < 0) {
            next[c] = 0;
        } else {
            fail[t] = 0;
            queue[tail++] = t;
        }
    }
    while (head < tail) {
        int32_t s = queue[head++];
        if (out[s] == 0) {
            out[s] = out[fail[s]];
        }
        for (int c = 0; c < 256; c++) {
            int32_t t = next[s * 256 + c];
            if (t < 0) {
                next[s * 256 + c] = next[fail[s] * 256 + c];
            } else {
                fail[t] = next[fail[s] * 256 + c];
                queue[tail++] = t;
            }
        }
    }
    int32_t s = 0;
    const unsigned char *text = (const unsigned char *)in->text;
    for (size_t i = 0; i < in->len; i++) {
        s = next[s * 256 + text[i]];
        if (out[s] > 0) {
            f->matches++;
            f->offsets += i + 1 - (size_t)out[s];
        }
    }
    free(next);
    free(fail);
    free(out);
    free(queue);
    return true;
}

static bool run_way(const Args *args, Result *res, size_t k, bool (*way)(const Input *, Found *)) {
    Input in;
    words_input(args->size, k, &in);
    Found f = {0};
    bool ok = true;
    for (size_t r = 0; r < args->repeats && ok; r++) {
        f = (Found){0};
        double start = now_ms();
        ok = way(&in, &f);
        res->times[r] = now_ms() - start;
    }
    snprintf(res->checksum, CHECKSUM_MAX, "%zu:%" PRIu64 ":%" PRIu64, k, f.matches, f.offsets);
    free(in.text);
    free(in.words);
    return ok;
}

#define WAYS_FOR(K)                                                                                \
    static bool strstr_##K(const Args *a, Result *r) {                                             \
        return run_way(a, r, K, strstr_loop);                                                      \
    }                                                                                              \
    static bool pcre2_##K(const Args *a, Result *r) {                                              \
        return run_way(a, r, K, pcre2_alternation);                                                \
    }                                                                                              \
    static bool hand_##K(const Args *a, Result *r) {                                               \
        return run_way(a, r, K, hand_aho_corasick);                                                \
    }

WAYS_FOR(1)
WAYS_FOR(10)
WAYS_FOR(100)
WAYS_FOR(1000)

#define ENTRIES(K)                                                                                 \
    {"bwords-" #K ".c.strstr", strstr_##K}, {"bwords-" #K ".c.pcre2", pcre2_##K}, {                \
        "bwords-" #K ".c.hand.aho-corasick", hand_##K                                              \
    }

const Kernel WORDS_KERNELS[] = {
    ENTRIES(1), ENTRIES(10), ENTRIES(100), ENTRIES(1000), {NULL, NULL},
};
