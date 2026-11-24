/* Autocomplete kernels (../../SPEC.md section 15): the first 10 words that start with a typed
 * prefix, from a sorted list with binary search, a 26-slot pointer trie, or the same trie in one
 * growable array with 32-bit links. Reference: python/autocomplete.py. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernels.h"

#define WORDS_FILE "words-web2.txt"
#define TOP_K 10
#define MAX_PREFIX 5
#define LETTERS 26
#define FIRST_CAPACITY 16

/* The word list: `items` point into `text`, one NUL-terminated word each. */
typedef struct {
    char *text;
    const char **items;
    size_t len;
} Words;

/* The results of one query or of a full walk. */
typedef struct {
    const char **items;
    size_t len;
} Out;

typedef struct {
    char text[MAX_PREFIX + 1];
    size_t len;
} Query;

static bool load_words(const char *dir, Words *w) {
    char *path = path_join(dir, WORDS_FILE);
    size_t len;
    unsigned char *raw = read_file(path, &len);
    free(path);
    if (raw == NULL) {
        return false;
    }
    char *text = xrealloc(raw, len + 1);
    text[len] = '\0';
    size_t lines = 0;
    for (size_t i = 0; i < len; i++) {
        lines += text[i] == '\n';
    }
    const char **items = xmalloc((lines + 1) * sizeof *items);
    size_t n = 0;
    for (char *line = text; *line != '\0';) {
        char *end = strchr(line, '\n');
        char *next = end != NULL ? end + 1 : line + strlen(line);
        if (end == NULL) {
            end = next;
        }
        if (end > line && end[-1] == '\r') {
            end--;
        }
        *end = '\0';
        if (end > line) {
            items[n++] = line;
        }
        line = next;
    }
    *w = (Words){text, items, n};
    return true;
}

static void shuffle(Words *w, Rng *rng) {
    for (size_t i = w->len - 1; i > 0; i--) {
        size_t j = rng_next(rng) % (i + 1);
        const char *t = w->items[i];
        w->items[i] = w->items[j];
        w->items[j] = t;
    }
}

static void free_words(Words *w) {
    free(w->items);
    free(w->text);
}

static uint32_t fold(uint32_t h, const char *word) {
    size_t len = strlen(word);
    h = h31_step(h, (int64_t)len);
    return h31_step(h, (unsigned char)word[len - 1]);
}

static uint32_t fold_all(uint32_t h, const Out *out) {
    for (size_t i = 0; i < out->len; i++) {
        h = fold(h, out->items[i]);
    }
    return h;
}

static int compare_words(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

/* The first index whose word is not less than the prefix. */
static size_t lower_bound(const char **words, size_t n, const char *prefix) {
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (strcmp(words[mid], prefix) < 0) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

static void sorted_complete(const char **sorted, size_t n, const Query *q, Out *out) {
    size_t i = lower_bound(sorted, n, q->text);
    size_t end = i + TOP_K < n ? i + TOP_K : n;
    while (i < end && strncmp(sorted[i], q->text, q->len) == 0) {
        out->items[out->len++] = sorted[i++];
    }
}

typedef struct TrieNode {
    struct TrieNode *children[LETTERS];
    int32_t word;
} TrieNode;

typedef struct {
    TrieNode *root;
    size_t nodes;
} Trie;

static TrieNode *trie_node(void) {
    TrieNode *node = xcalloc(1, sizeof *node);
    node->word = -1;
    return node;
}

static Trie trie_build(const Words *w) {
    Trie t = {trie_node(), 1};
    for (size_t idx = 0; idx < w->len; idx++) {
        TrieNode *node = t.root;
        for (const char *ch = w->items[idx]; *ch != '\0'; ch++) {
            TrieNode **child = &node->children[*ch - 'a'];
            if (*child == NULL) {
                *child = trie_node();
                t.nodes++;
            }
            node = *child;
        }
        node->word = (int32_t)idx;
    }
    return t;
}

static void trie_free(TrieNode *node) {
    if (node == NULL) {
        return;
    }
    for (int c = 0; c < LETTERS; c++) {
        trie_free(node->children[c]);
    }
    free(node);
}

static void collect(const TrieNode *node, const Words *w, Out *out, size_t limit) {
    if (node->word >= 0) {
        out->items[out->len++] = w->items[node->word];
    }
    for (int c = 0; c < LETTERS; c++) {
        if (out->len >= limit) {
            return;
        }
        if (node->children[c] != NULL) {
            collect(node->children[c], w, out, limit);
        }
    }
}

static void trie_complete(const TrieNode *root, const Words *w, const Query *q, Out *out) {
    const TrieNode *node = root;
    for (const char *ch = q->text; *ch != '\0'; ch++) {
        node = node->children[*ch - 'a'];
        if (node == NULL) {
            return;
        }
    }
    collect(node, w, out, TOP_K);
}

/* Node 0 is the root and never anyone's child, so a child link of 0 means no child. */
typedef struct {
    uint32_t children[LETTERS];
    int32_t word;
} ArenaNode;

typedef struct {
    ArenaNode *nodes;
    size_t len, cap;
} Arena;

static uint32_t arena_push(Arena *a) {
    if (a->len == a->cap) {
        a->cap = a->cap == 0 ? FIRST_CAPACITY : a->cap * 2;
        a->nodes = xrealloc(a->nodes, a->cap * sizeof *a->nodes);
    }
    ArenaNode *node = &a->nodes[a->len];
    memset(node->children, 0, sizeof node->children);
    node->word = -1;
    return (uint32_t)a->len++;
}

static Arena arena_build(const Words *w) {
    Arena a = {0};
    arena_push(&a);
    for (size_t idx = 0; idx < w->len; idx++) {
        uint32_t node = 0;
        for (const char *ch = w->items[idx]; *ch != '\0'; ch++) {
            int c = *ch - 'a';
            if (a.nodes[node].children[c] == 0) {
                uint32_t child = arena_push(&a);
                a.nodes[node].children[c] = child;
            }
            node = a.nodes[node].children[c];
        }
        a.nodes[node].word = (int32_t)idx;
    }
    return a;
}

static void arena_collect(const Arena *a, uint32_t node, const Words *w, Out *out, size_t limit) {
    const ArenaNode *n = &a->nodes[node];
    if (n->word >= 0) {
        out->items[out->len++] = w->items[n->word];
    }
    for (int c = 0; c < LETTERS; c++) {
        if (out->len >= limit) {
            return;
        }
        if (n->children[c] != 0) {
            arena_collect(a, n->children[c], w, out, limit);
        }
    }
}

static void arena_complete(const Arena *a, const Words *w, const Query *q, Out *out) {
    uint32_t node = 0;
    for (const char *ch = q->text; *ch != '\0'; ch++) {
        node = a->nodes[node].children[*ch - 'a'];
        if (node == 0) {
            return;
        }
    }
    arena_collect(a, node, w, out, TOP_K);
}

/* The shuffled words of SPEC.md section 15, cut to the first `n` (all of them when n is larger),
 * and the generator, ready to draw the queries. */
static bool shuffled_words(const Args *args, size_t n, Words *w, Rng *rng) {
    if (!load_words(args->data, w)) {
        return false;
    }
    *rng = rng_new();
    shuffle(w, rng);
    if (n < w->len) {
        w->len = n;
    }
    return true;
}

static void build_checksum(Result *out, const Out *in_order) {
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%" PRIu32, in_order->len, fold_all(0, in_order));
}

static bool sorted_build(const Args *args, Result *out) {
    Words w;
    Rng rng;
    if (!shuffled_words(args, args->size, &w, &rng)) {
        return false;
    }
    Out sorted = {xmalloc(w.len * sizeof(char *)), w.len};
    for (size_t r = 0; r < args->repeats; r++) {
        memcpy(sorted.items, w.items, w.len * sizeof(char *));
        double start = now_ms();
        qsort(sorted.items, sorted.len, sizeof(char *), compare_words);
        out->times[r] = now_ms() - start;
    }
    build_checksum(out, &sorted);
    free(sorted.items);
    free_words(&w);
    return true;
}

static void trie_build_checksum(Result *out, size_t nodes, const Out *walk) {
    build_checksum(out, walk);
    snprintf(out->extra, EXTRA_MAX, "{\"nodes\":%zu}", nodes);
}

static bool trie_build_kernel(const Args *args, Result *out) {
    Words w;
    Rng rng;
    if (!shuffled_words(args, args->size, &w, &rng)) {
        return false;
    }
    Trie t = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        trie_free(t.root);
        double start = now_ms();
        t = trie_build(&w);
        out->times[r] = now_ms() - start;
    }
    Out walk = {xmalloc(w.len * sizeof(char *)), 0};
    collect(t.root, &w, &walk, w.len + 1);
    trie_build_checksum(out, t.nodes, &walk);
    free(walk.items);
    trie_free(t.root);
    free_words(&w);
    return true;
}

static bool arena_build_kernel(const Args *args, Result *out) {
    Words w;
    Rng rng;
    if (!shuffled_words(args, args->size, &w, &rng)) {
        return false;
    }
    Arena a = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        free(a.nodes);
        double start = now_ms();
        a = arena_build(&w);
        out->times[r] = now_ms() - start;
    }
    Out walk = {xmalloc(w.len * sizeof(char *)), 0};
    arena_collect(&a, 0, &w, &walk, w.len + 1);
    trie_build_checksum(out, a.len, &walk);
    free(walk.items);
    free(a.nodes);
    free_words(&w);
    return true;
}

static Query *make_queries(const Words *w, Rng *rng, size_t n) {
    Query *queries = xmalloc(n * sizeof *queries);
    for (size_t i = 0; i < n; i++) {
        const char *word = w->items[rng_next(rng) % w->len];
        size_t p = 2 + rng_next(rng) % 4;
        size_t len = strlen(word);
        queries[i].len = p < len ? p : len;
        memcpy(queries[i].text, word, queries[i].len);
        queries[i].text[queries[i].len] = '\0';
    }
    return queries;
}

typedef enum { BY_SORTED, BY_TRIE, BY_ARENA } Method;

typedef struct {
    Method method;
    const Words *words;
    const char **sorted;
    const TrieNode *root;
    const Arena *arena;
} Index;

static void complete(const Index *ix, const Query *q, Out *out) {
    switch (ix->method) {
    case BY_SORTED:
        sorted_complete(ix->sorted, ix->words->len, q, out);
        break;
    case BY_TRIE:
        trie_complete(ix->root, ix->words, q, out);
        break;
    case BY_ARENA:
        arena_complete(ix->arena, ix->words, q, out);
        break;
    }
}

static bool run_queries(const Args *args, Result *out, Method method) {
    Words w;
    Rng rng;
    if (!shuffled_words(args, SIZE_MAX, &w, &rng)) {
        return false;
    }
    Query *queries = make_queries(&w, &rng, args->size);
    Index ix = {method, &w, NULL, NULL, NULL};
    Trie t = {0};
    Arena a = {0};
    if (method == BY_SORTED) {
        ix.sorted = xmalloc(w.len * sizeof(char *));
        memcpy(ix.sorted, w.items, w.len * sizeof(char *));
        qsort(ix.sorted, w.len, sizeof(char *), compare_words);
    } else if (method == BY_TRIE) {
        t = trie_build(&w);
        ix.root = t.root;
    } else {
        a = arena_build(&w);
        ix.arena = &a;
    }
    Out found = {xmalloc(TOP_K * sizeof(char *)), 0};
    size_t results = 0;
    uint32_t h = 0;
    for (size_t r = 0; r < args->repeats; r++) {
        results = 0;
        h = 0;
        double start = now_ms();
        for (size_t i = 0; i < args->size; i++) {
            found.len = 0;
            complete(&ix, &queries[i], &found);
            results += found.len;
            h = fold_all(h, &found);
        }
        out->times[r] = now_ms() - start;
    }
    sink = h;
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu:%" PRIu32, args->size, results, h);
    free(found.items);
    free(ix.sorted);
    trie_free(t.root);
    free(a.nodes);
    free(queries);
    free_words(&w);
    return true;
}

static bool sorted_query(const Args *args, Result *out) {
    return run_queries(args, out, BY_SORTED);
}

static bool trie_query(const Args *args, Result *out) {
    return run_queries(args, out, BY_TRIE);
}

static bool arena_query(const Args *args, Result *out) {
    return run_queries(args, out, BY_ARENA);
}

const Kernel AUTOCOMPLETE_KERNELS[] = {
    {"ac-sorted-build", sorted_build},
    {"ac-trie-build", trie_build_kernel},
    {"ac-trie-arena-build", arena_build_kernel},
    {"ac-sorted", sorted_query},
    {"ac-trie", trie_query},
    {"ac-trie-arena", arena_query},
    {NULL, NULL},
};
