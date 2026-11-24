// Race: binary-trees-arena (c)
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

/* A binary tree's nodes, children pushed before their parent; a leaf's links are NIL. */
typedef struct {
    uint32_t left;
    uint32_t right;
} TreeNode;

typedef struct {
    TreeNode *data;
    size_t len, cap;
} TreeArena;

static uint32_t make_tree(TreeArena *a, unsigned depth) {
    TreeNode node = {NIL, NIL};
    if (depth > 0) {
        node.left = make_tree(a, depth - 1);
        node.right = make_tree(a, depth - 1);
    }
    a->data = grow(a->data, a->len, &a->cap, sizeof *a->data);
    a->data[a->len] = node;
    return (uint32_t)a->len++;
}

static uint64_t check_tree(const TreeArena *a, uint32_t index) {
    TreeNode node = a->data[index];
    if (node.left == NIL) {
        return 1;
    }
    return 1 + check_tree(a, node.left) + check_tree(a, node.right);
}

static uint64_t tree_check(unsigned depth) {
    TreeArena a = {0};
    uint32_t root = make_tree(&a, depth);
    uint64_t check = check_tree(&a, root);
    free(a.data);
    return check;
}

static void binary_trees_arena_text(unsigned n, Bytes *out) {
    unsigned max_depth = n > MIN_DEPTH + 2 ? n : MIN_DEPTH + 2;
    unsigned stretch = max_depth + 1;
    out->len = 0;
    bytes_printf(out, "stretch tree of depth %u\t check: %" PRIu64 "\n", stretch,
                 tree_check(stretch));
    TreeArena long_lived = {0};
    uint32_t long_root = make_tree(&long_lived, max_depth);
    for (unsigned depth = MIN_DEPTH; depth <= max_depth; depth += 2) {
        uint64_t iterations = (uint64_t)1 << (max_depth - depth + MIN_DEPTH);
        uint64_t check = 0;
        for (uint64_t i = 0; i < iterations; i++) {
            check += tree_check(depth);
        }
        bytes_printf(out, "%" PRIu64 "\t trees of depth %u\t check: %" PRIu64 "\n", iterations,
                     depth, check);
    }
    bytes_printf(out, "long lived tree of depth %u\t check: %" PRIu64 "\n", max_depth,
                 check_tree(&long_lived, long_root));
    free(long_lived.data);
}

static bool binary_trees_arena(const Args *args, Result *out) {
    /* 2^(depth + 2) nodes in the stretch tree must stay below the NIL link. */
    if (args->size > 29) {
        fprintf(stderr, "error: --size %zu is too deep for 32-bit links\n", args->size);
        return false;
    }
    Bytes text = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        binary_trees_arena_text((unsigned)args->size, &text);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu32, fnv1a(text.data, text.len));
    free(text.data);
    return true;
}
