// Race: trie (c)
// Extracted from suite/c/src/algo_tree.c — build and run via suite/run.py

/* Pointer-structure kernels of group `algo` (SPEC.md section 12): bst, trie, lru. Every node is one
 * malloc, and the structure is freed inside the timed call, as Rust drops its Boxes there. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "algo_tree.h"
#include "intmap.h"
#include "kernels.h"

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
