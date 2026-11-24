// Race: trie-arena (c)
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

#define FIRST_CAPACITY 16

/* Room for one more element of `width` bytes in *data (capacity *cap), doubling when full. */
static void *grow(void *data, size_t len, size_t *cap, size_t width) {
    if (len < *cap) {
        return data;
    }
    *cap = *cap == 0 ? FIRST_CAPACITY : *cap * 2;
    return xrealloc(data, *cap * width);
}

/* Node 0 is the root and never anyone's child, so a child link of 0 means no child. */
typedef struct {
    uint32_t count;
    uint32_t children[26];
} TrieNode;

typedef struct {
    TrieNode *data;
    size_t len, cap;
} TrieArena;

static void trie_push(TrieArena *a) {
    a->data = grow(a->data, a->len, &a->cap, sizeof *a->data);
    memset(&a->data[a->len++], 0, sizeof *a->data);
}

typedef struct {
    size_t nodes;
    uint64_t total;
} TrieResult;

static TrieResult trie_arena_run(const Words *w, size_t n) {
    TrieArena a = {0};
    trie_push(&a);
    for (size_t i = 0; i < n; i++) {
        uint32_t node = 0;
        for (const char *ch = w->words[i]; *ch != '\0'; ch++) {
            int c = *ch - 'a';
            if (a.data[node].children[c] == 0) {
                a.data[node].children[c] = (uint32_t)a.len;
                trie_push(&a);
            }
            node = a.data[node].children[c];
            a.data[node].count++;
        }
    }
    uint64_t total = 0;
    for (size_t i = 0; i < n; i++) {
        /* A prefix has at least one letter, so node 0 after the walk means it is absent. */
        uint32_t node = 0;
        for (const char *ch = w->prefixes[i]; *ch != '\0'; ch++) {
            node = a.data[node].children[*ch - 'a'];
            if (node == 0) {
                break;
            }
        }
        if (node != 0) {
            total += a.data[node].count;
        }
    }
    TrieResult res = {a.len - 1, total};
    free(a.data);
    return res;
}

static bool trie_arena(const Args *args, Result *out) {
    Words w = words(args->size);
    TrieResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = trie_arena_run(&w, args->size);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%" PRIu64, res.nodes, res.total);
    free(w.words);
    free(w.prefixes);
    free(w.block);
    return true;
}
