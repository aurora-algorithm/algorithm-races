// Race: binary-trees (c)
// Extracted from suite/c/src/bg.c — build and run via suite/run.py

/* Benchmarks Game kernels, single-threaded (SPEC.md section 4). Floating-point code keeps the
 * operation order of the spec so every language prints the same digits. */
#include "bg.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

typedef struct Node {
    struct Node *left;
    struct Node *right;
} Node;

static Node *make_tree(unsigned depth) {
    Node *node = xmalloc(sizeof *node);
    if (depth == 0) {
        node->left = node->right = NULL;
    } else {
        node->left = make_tree(depth - 1);
        node->right = make_tree(depth - 1);
    }
    return node;
}

static uint64_t check_tree(const Node *node) {
    if (node->left == NULL) {
        return 1;
    }
    return 1 + check_tree(node->left) + check_tree(node->right);
}

static void free_tree(Node *node) {
    if (node->left != NULL) {
        free_tree(node->left);
        free_tree(node->right);
    }
    free(node);
}

uint64_t depth_checks(unsigned depth, uint64_t iterations) {
    uint64_t sum = 0;
    for (uint64_t i = 0; i < iterations; i++) {
        Node *tree = make_tree(depth);
        sum += check_tree(tree);
        free_tree(tree);
    }
    return sum;
}

void binary_trees_text(unsigned n, DepthRun run, const void *ctx, Bytes *out) {
    unsigned max_depth = n > MIN_DEPTH + 2 ? n : MIN_DEPTH + 2;
    unsigned stretch = max_depth + 1;
    out->len = 0;
    bytes_printf(out, "stretch tree of depth %u\t check: %" PRIu64 "\n", stretch,
                 depth_checks(stretch, 1));
    Node *long_lived = make_tree(max_depth);
    for (unsigned depth = MIN_DEPTH; depth <= max_depth; depth += 2) {
        uint64_t iterations = (uint64_t)1 << (max_depth - depth + MIN_DEPTH);
        uint64_t check = run(depth, iterations, ctx);
        bytes_printf(out, "%" PRIu64 "\t trees of depth %u\t check: %" PRIu64 "\n", iterations,
                     depth, check);
    }
    bytes_printf(out, "long lived tree of depth %u\t check: %" PRIu64 "\n", max_depth,
                 check_tree(long_lived));
    free_tree(long_lived);
}

bool run_binary_trees(const Args *args, Result *out, DepthRun run) {
    if (args->size > 40) {
        fprintf(stderr, "error: --size %zu is too deep for a tree\n", args->size);
        return false;
    }
    Bytes text = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        binary_trees_text((unsigned)args->size, run, &args->threads, &text);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu32, fnv1a(text.data, text.len));
    free(text.data);
    return true;
}

static uint64_t sequential_depth(unsigned depth, uint64_t iterations, const void *ctx) {
    (void)ctx;
    return depth_checks(depth, iterations);
}

static bool binary_trees(const Args *args, Result *out) {
    return run_binary_trees(args, out, sequential_depth);
}
