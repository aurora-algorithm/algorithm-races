/* Pointer-structure kernels of group `algo` (SPEC.md section 12): bst, trie, lru. Every node is one
 * malloc, and the structure is freed inside the timed call, as Rust drops its Boxes there. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "algo_tree.h"
#include "intmap.h"
#include "kernels.h"

typedef struct BstNode {
    int64_t key;
    uint32_t count;
    struct BstNode *left;
    struct BstNode *right;
} BstNode;

static void bst_insert(BstNode **root, int64_t key, size_t *size) {
    BstNode **slot = root;
    while (*slot != NULL) {
        BstNode *node = *slot;
        if (key < node->key) {
            slot = &node->left;
        } else if (key > node->key) {
            slot = &node->right;
        } else {
            node->count++;
            return;
        }
    }
    BstNode *node = xmalloc(sizeof *node);
    *node = (BstNode){key, 1, NULL, NULL};
    *slot = node;
    (*size)++;
}

static bool bst_contains(const BstNode *node, int64_t key) {
    while (node != NULL) {
        if (key < node->key) {
            node = node->left;
        } else if (key > node->key) {
            node = node->right;
        } else {
            return true;
        }
    }
    return false;
}

static void bst_free(BstNode *node) {
    if (node != NULL) {
        bst_free(node->left);
        bst_free(node->right);
        free(node);
    }
}

/* The keys in order, walked with an explicit stack. */
static int64_t *bst_keys(const BstNode *root, size_t size) {
    const BstNode **stack = xmalloc((size + 1) * sizeof *stack);
    int64_t *keys = xmalloc((size + 1) * sizeof *keys);
    size_t depth = 0, len = 0;
    const BstNode *node = root;
    while (node != NULL || depth > 0) {
        while (node != NULL) {
            stack[depth++] = node;
            node = node->left;
        }
        node = stack[--depth];
        keys[len++] = node->key;
        node = node->right;
    }
    free(stack);
    return keys;
}

typedef struct {
    size_t size, hits;
    int64_t *keys;
} KeysResult;

static KeysResult bst_run(const int64_t *inserts, const int64_t *lookups, size_t n) {
    BstNode *root = NULL;
    size_t size = 0, hits = 0;
    for (size_t i = 0; i < n; i++) {
        bst_insert(&root, inserts[i], &size);
    }
    for (size_t i = 0; i < n; i++) {
        hits += bst_contains(root, lookups[i]);
    }
    int64_t *keys = bst_keys(root, size);
    bst_free(root);
    return (KeysResult){size, hits, keys};
}

/* "{a}:{b}:{h31 of keys}", then frees the keys. */
static void keys_checksum(KeysResult *res, char *checksum) {
    snprintf(checksum, CHECKSUM_MAX, "%zu:%zu:%" PRIu32, res->size, res->hits,
             h31(res->keys, res->size));
    free(res->keys);
    res->keys = NULL;
}

static bool bst(const Args *args, Result *out) {
    int64_t *inserts, *lookups;
    hashmap_input(args->size, &inserts, &lookups);
    KeysResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        free(res.keys);
        double start = now_ms();
        res = bst_run(inserts, lookups, args->size);
        out->times[r] = now_ms() - start;
    }
    keys_checksum(&res, out->checksum);
    free(inserts);
    free(lookups);
    return true;
}

static char *random_word(Rng *rng, uint32_t min_len, uint32_t spread, char *p) {
    uint32_t len = min_len + rng_next(rng) % spread;
    for (uint32_t i = 0; i < len; i++) {
        *p++ = (char)('a' + rng_next(rng) % 26);
    }
    *p++ = '\0';
    return p;
}

Words words(size_t n) {
    Rng rng = rng_new();
    Words w = {xmalloc(n * sizeof(char *)), xmalloc(n * sizeof(char *)), xmalloc(n * 16)};
    char *p = w.block;
    for (size_t i = 0; i < n; i++) {
        w.words[i] = p;
        p = random_word(&rng, 3, 8, p);
    }
    for (size_t i = 0; i < n; i++) {
        w.prefixes[i] = p;
        p = random_word(&rng, 1, 4, p);
    }
    return w;
}

typedef struct TrieNode {
    uint64_t count;
    struct TrieNode *children[26];
} TrieNode;

static void trie_free(TrieNode *node) {
    for (int c = 0; c < 26; c++) {
        if (node->children[c] != NULL) {
            trie_free(node->children[c]);
        }
    }
    free(node);
}

typedef struct {
    size_t nodes;
    uint64_t total;
} TrieResult;

static TrieResult trie_run(const Words *w, size_t n) {
    TrieNode *root = xcalloc(1, sizeof *root);
    size_t nodes = 0;
    for (size_t i = 0; i < n; i++) {
        TrieNode *node = root;
        for (const char *ch = w->words[i]; *ch != '\0'; ch++) {
            TrieNode **child = &node->children[*ch - 'a'];
            if (*child == NULL) {
                *child = xcalloc(1, sizeof **child);
                nodes++;
            }
            node = *child;
            node->count++;
        }
    }
    uint64_t total = 0;
    for (size_t i = 0; i < n; i++) {
        const TrieNode *node = root;
        for (const char *ch = w->prefixes[i]; *ch != '\0' && node != NULL; ch++) {
            node = node->children[*ch - 'a'];
        }
        total += node != NULL ? node->count : 0;
    }
    trie_free(root);
    return (TrieResult){nodes, total};
}

static bool trie(const Args *args, Result *out) {
    Words w = words(args->size);
    TrieResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = trie_run(&w, args->size);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%" PRIu64, res.nodes, res.total);
    free(w.words);
    free(w.prefixes);
    free(w.block);
    return true;
}

/* One cache entry in a doubly linked list, most recent at the head. */
typedef struct LruNode {
    uint32_t key;
    struct LruNode *prev;
    struct LruNode *next;
} LruNode;

typedef struct {
    IntMap nodes; /* key -> LruNode*, stored as an integer */
    LruNode *head;
    LruNode *tail;
} Lru;

static void lru_unlink(Lru *c, LruNode *e) {
    if (e->prev != NULL) {
        e->prev->next = e->next;
    } else {
        c->head = e->next;
    }
    if (e->next != NULL) {
        e->next->prev = e->prev;
    } else {
        c->tail = e->prev;
    }
}

static void lru_push_front(Lru *c, LruNode *e) {
    e->prev = NULL;
    e->next = c->head;
    if (c->head != NULL) {
        c->head->prev = e;
    } else {
        c->tail = e;
    }
    c->head = e;
}

typedef struct {
    size_t hits, misses, len;
    int64_t *keys;
} LruResult;

static LruResult lru_run(size_t capacity, const uint32_t *keys, size_t n) {
    Lru c = {intmap_with_capacity(capacity), NULL, NULL};
    size_t hits = 0;
    for (size_t i = 0; i < n; i++) {
        int64_t *slot = intmap_get(&c.nodes, keys[i]);
        if (slot != NULL) {
            LruNode *e = (LruNode *)(intptr_t)*slot;
            lru_unlink(&c, e);
            lru_push_front(&c, e);
            hits++;
            continue;
        }
        if (c.nodes.len == capacity) {
            LruNode *oldest = c.tail;
            intmap_remove(&c.nodes, oldest->key);
            lru_unlink(&c, oldest);
            free(oldest);
        }
        LruNode *e = xmalloc(sizeof *e);
        e->key = keys[i];
        lru_push_front(&c, e);
        *intmap_entry(&c.nodes, keys[i]) = (int64_t)(intptr_t)e;
    }
    int64_t *cached = xmalloc((c.nodes.len + 1) * sizeof *cached);
    size_t len = 0;
    for (LruNode *e = c.tail; e != NULL;) {
        LruNode *prev = e->prev;
        cached[len++] = e->key;
        free(e);
        e = prev;
    }
    intmap_free(&c.nodes);
    return (LruResult){hits, n - hits, len, cached};
}

static bool lru(const Args *args, Result *out) {
    size_t n = args->size;
    size_t capacity = n / 100 > 1 ? n / 100 : 1;
    uint32_t *keys = xmalloc(n * sizeof *keys);
    Rng rng = rng_new();
    for (size_t i = 0; i < n; i++) {
        keys[i] = rng_next(&rng) % (uint32_t)(2 * capacity);
    }
    LruResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        free(res.keys);
        double start = now_ms();
        res = lru_run(capacity, keys, n);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu:%" PRIu32, res.hits, res.misses,
             h31(res.keys, res.len));
    free(res.keys);
    free(keys);
    return true;
}

const Kernel TREE_KERNELS[] = {{"bst", bst}, {"trie", trie}, {"lru", lru}, {NULL, NULL}};
