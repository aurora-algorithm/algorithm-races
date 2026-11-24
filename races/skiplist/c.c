// Race: skiplist (c)
// Extracted from suite/c/src/puzzle.c — build and run via suite/run.py

/* Unusual algorithm kernels (SPEC.md section 13, group `puzzle`): one clever trick each. The string
 * kernels are in puzzle_text.c, sudoku in sudoku.c. */
#include "puzzle.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

#define SKIP_MAX_LEVEL 24

#define SKIP_LEVEL_SEED 88675123u

typedef struct SkipNode {
    int64_t key;
    struct SkipNode *next[]; /* one link per level of this node's tower */
} SkipNode;

typedef struct {
    SkipNode *head;
    size_t level, hits, heights;
} SkipList;

static SkipNode *skip_node(int64_t key, size_t height) {
    SkipNode *node = xmalloc(sizeof *node + height * sizeof(SkipNode *));
    node->key = key;
    return node;
}

/* Walks from the top level down to the last node with a smaller key, noting where it dropped a
 * level. */
static SkipNode *skip_find(const SkipList *list, int64_t key, SkipNode **update) {
    SkipNode *x = list->head;
    for (size_t i = list->level; i-- > 0;) {
        while (x->next[i] != NULL && x->next[i]->key < key) {
            x = x->next[i];
        }
        update[i] = x;
    }
    SkipNode *next = x->next[0];
    return next != NULL && next->key == key ? next : NULL;
}

static void skip_insert(SkipList *list, int64_t key, Rng *rng) {
    SkipNode *update[SKIP_MAX_LEVEL];
    if (skip_find(list, key, update) != NULL) {
        return;
    }
    size_t height = 1;
    while (height < SKIP_MAX_LEVEL && (rng_next(rng) & 1) != 0) {
        height++;
    }
    for (size_t i = list->level; i < height; i++) {
        update[i] = list->head;
    }
    if (height > list->level) {
        list->level = height;
    }
    SkipNode *node = skip_node(key, height);
    for (size_t i = 0; i < height; i++) {
        node->next[i] = update[i]->next[i];
        update[i]->next[i] = node;
    }
    list->heights += height;
}

static SkipList skiplist_build(const int64_t *keys, const int64_t *lookups, size_t n) {
    SkipList list = {skip_node(-1, SKIP_MAX_LEVEL), 1, 0, 0};
    for (int i = 0; i < SKIP_MAX_LEVEL; i++) {
        list.head->next[i] = NULL;
    }
    Rng rng = {SKIP_LEVEL_SEED};
    for (size_t i = 0; i < n; i++) {
        skip_insert(&list, keys[i], &rng);
    }
    SkipNode *update[SKIP_MAX_LEVEL];
    for (size_t i = 0; i < n; i++) {
        list.hits += skip_find(&list, lookups[i], update) != NULL;
    }
    return list;
}

/* h31 of the keys along level 0, freeing the nodes on the way. */
static uint32_t skiplist_free(SkipList *list) {
    uint32_t h = 0;
    SkipNode *x = list->head->next[0];
    free(list->head);
    while (x != NULL) {
        SkipNode *next = x->next[0];
        h = h31_step(h, x->key);
        free(x);
        x = next;
    }
    return h;
}

static bool skiplist(const Args *args, Result *out) {
    size_t n = args->size;
    int64_t *keys, *lookups;
    hashmap_input(n, &keys, &lookups);
    SkipList list = {0};
    uint32_t h = 0;
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        list = skiplist_build(keys, lookups, n);
        out->times[r] = now_ms() - start;
        h = skiplist_free(&list);
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu:%zu:%" PRIu32, n, list.hits, list.heights, h);
    free(keys);
    free(lookups);
    return true;
}
