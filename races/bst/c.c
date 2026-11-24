// Race: bst (c)
// Extracted from suite/c/src/algo_tree.c — build and run via suite/run.py

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
