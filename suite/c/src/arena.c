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

const Kernel ARENA_KERNELS[] = {
    {"bst-arena", bst_arena},
    {"trie-arena", trie_arena},
    {"binary-trees-arena", binary_trees_arena},
    {NULL, NULL},
};
