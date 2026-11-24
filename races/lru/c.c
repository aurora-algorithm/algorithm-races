// Race: lru (c)
// Extracted from suite/c/src/algo_tree.c — build and run via suite/run.py

/* Pointer-structure kernels of group `algo` (SPEC.md section 12): bst, trie, lru. Every node is one
 * malloc, and the structure is freed inside the timed call, as Rust drops its Boxes there. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "algo_tree.h"
#include "intmap.h"
#include "kernels.h"

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
