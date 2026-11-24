// Race: bst-arena (c)
// Extracted from suite/c/src/arena.c — build and run via suite/run.py

/* The -arena kernels (../../SPEC.md section 12, "Arena rows"): bst, trie and binary-trees with
 * every node in one growable array (realloc, doubling from 16) and 32-bit index links, the same
 * algorithms and node layouts as rust/src/arena.rs and go/arena.go. Each array is freed inside the
 * timed call, as Rust drops its Vec there. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "algo_tree.h"
#include "bg.h"
#include "kernels.h"

/* The "no child" link of the BST and the binary trees. */
#define NIL UINT32_MAX

#define FIRST_CAPACITY 16

/* Room for one more element of `width` bytes in *data (capacity *cap), doubling when full. */
static void *grow(void *data, size_t len, size_t *cap, size_t width) {
    if (len < *cap) {
        return data;
    }
    *cap = *cap == 0 ? FIRST_CAPACITY : *cap * 2;
    return xrealloc(data, *cap * width);
}

typedef struct {
    int64_t key;
    uint32_t count;
    uint32_t left;
    uint32_t right;
} BstNode;

typedef struct {
    BstNode *data;
    size_t len, cap;
} BstArena;

static void bst_push(BstArena *a, int64_t key) {
    a->data = grow(a->data, a->len, &a->cap, sizeof *a->data);
    a->data[a->len++] = (BstNode){key, 1, NIL, NIL};
}

static void bst_insert(BstArena *a, int64_t key) {
    if (a->len == 0) {
        bst_push(a, key);
        return;
    }
    uint32_t next = (uint32_t)a->len;
    uint32_t cur = 0;
    for (;;) {
        BstNode *node = &a->data[cur];
        if (key == node->key) {
            node->count++;
            return;
        }
        uint32_t *link = key < node->key ? &node->left : &node->right;
        if (*link == NIL) {
            *link = next;
            bst_push(a, key);
            return;
        }
        cur = *link;
    }
}

static bool bst_contains(const BstArena *a, int64_t key) {
    uint32_t cur = a->len == 0 ? NIL : 0;
    while (cur != NIL && a->data[cur].key != key) {
        const BstNode *node = &a->data[cur];
        cur = key < node->key ? node->left : node->right;
    }
    return cur != NIL;
}

/* The keys in order, walked with an explicit stack of indexes. */
static int64_t *bst_keys(const BstArena *a) {
    uint32_t *stack = xmalloc((a->len + 1) * sizeof *stack);
    int64_t *keys = xmalloc((a->len + 1) * sizeof *keys);
    size_t depth = 0, len = 0;
    uint32_t cur = a->len == 0 ? NIL : 0;
    while (cur != NIL || depth > 0) {
        while (cur != NIL) {
            stack[depth++] = cur;
            cur = a->data[cur].left;
        }
        uint32_t top = stack[--depth];
        keys[len++] = a->data[top].key;
        cur = a->data[top].right;
    }
    free(stack);
    return keys;
}

typedef struct {
    size_t size, hits;
    int64_t *keys;
} BstResult;

static BstResult bst_arena_run(const int64_t *inserts, const int64_t *lookups, size_t n) {
    BstArena a = {0};
    for (size_t i = 0; i < n; i++) {
        bst_insert(&a, inserts[i]);
    }
    size_t hits = 0;
    for (size_t i = 0; i < n; i++) {
        hits += bst_contains(&a, lookups[i]);
    }
    BstResult res = {a.len, hits, bst_keys(&a)};
    free(a.data);
    return res;
}

static bool bst_arena(const Args *args, Result *out) {
    if (args->size >= NIL) {
        fprintf(stderr, "error: --size %zu does not fit 32-bit links\n", args->size);
        return false;
    }
    int64_t *inserts, *lookups;
    hashmap_input(args->size, &inserts, &lookups);
    BstResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        free(res.keys);
        double start = now_ms();
        res = bst_arena_run(inserts, lookups, args->size);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu:%" PRIu32, res.size, res.hits,
             h31(res.keys, res.size));
    free(res.keys);
    free(inserts);
    free(lookups);
    return true;
}
