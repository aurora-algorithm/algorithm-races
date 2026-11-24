// Race: dijkstra (c)
// Extracted from suite/c/src/algo_graph.c — build and run via suite/run.py

/* Graph kernels of group `algo` (SPEC.md section 12): bfs-grid, dijkstra, kruskal. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

typedef struct {
    uint64_t dist;
    size_t node;
} HeapItem;

/* A binary min-heap of (distance, node) ordered by distance only: C has no priority queue in its
 * standard library. */
typedef struct {
    HeapItem *items;
    size_t len, cap;
} MinHeap;

static bool item_less(HeapItem a, HeapItem b) {
    return a.dist < b.dist;
}

static void heap_push(MinHeap *h, HeapItem item) {
    if (h->len == h->cap) {
        h->cap = h->cap ? h->cap * 2 : 16;
        h->items = xrealloc(h->items, h->cap * sizeof *h->items);
    }
    size_t i = h->len++;
    while (i > 0 && item_less(item, h->items[(i - 1) / 2])) {
        h->items[i] = h->items[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    h->items[i] = item;
}

static HeapItem heap_pop(MinHeap *h) {
    HeapItem top = h->items[0];
    HeapItem last = h->items[--h->len];
    size_t i = 0;
    for (;;) {
        size_t child = 2 * i + 1;
        if (child >= h->len) {
            break;
        }
        if (child + 1 < h->len && item_less(h->items[child + 1], h->items[child])) {
            child++;
        }
        if (!item_less(h->items[child], last)) {
            break;
        }
        h->items[i] = h->items[child];
        i = child;
    }
    if (h->len > 0) {
        h->items[i] = last;
    }
    return top;
}

/* Four edges per node u at 4u..4u+3: one to u+1 (a ring, so every node is reachable), three random.
 */
static void graph(size_t n, size_t **to, uint64_t **weight) {
    Rng rng = rng_new();
    *to = xmalloc(4 * n * sizeof **to);
    *weight = xmalloc(4 * n * sizeof **weight);
    for (size_t u = 0; u < n; u++) {
        (*to)[4 * u] = (u + 1) % n;
        (*weight)[4 * u] = 1 + rng_next(&rng) % 100;
        for (size_t e = 4 * u + 1; e < 4 * u + 4; e++) {
            (*to)[e] = rng_next(&rng) % n;
            (*weight)[e] = 1 + rng_next(&rng) % 100;
        }
    }
}

typedef struct {
    size_t reached;
    uint64_t total, max;
} DijkstraResult;

static DijkstraResult dijkstra_run(size_t n, const size_t *to, const uint64_t *weight) {
    uint64_t *dist = xmalloc(n * sizeof *dist);
    for (size_t i = 0; i < n; i++) {
        dist[i] = UINT64_MAX;
    }
    dist[0] = 0;
    MinHeap heap = {0};
    heap_push(&heap, (HeapItem){0, 0});
    while (heap.len > 0) {
        HeapItem top = heap_pop(&heap);
        if (top.dist > dist[top.node]) {
            continue;
        }
        for (size_t e = 4 * top.node; e < 4 * top.node + 4; e++) {
            size_t v = to[e];
            uint64_t nd = top.dist + weight[e];
            if (nd < dist[v]) {
                dist[v] = nd;
                heap_push(&heap, (HeapItem){nd, v});
            }
        }
    }
    size_t reached = 0;
    uint64_t total = 0, max = 0;
    for (size_t i = 0; i < n; i++) {
        if (dist[i] < UINT64_MAX) {
            reached++;
            total += dist[i];
            max = dist[i] > max ? dist[i] : max;
        }
    }
    free(heap.items);
    free(dist);
    return (DijkstraResult){reached, total, max};
}

static bool dijkstra(const Args *args, Result *out) {
    size_t *to;
    uint64_t *weight;
    graph(args->size, &to, &weight);
    DijkstraResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = dijkstra_run(args->size, to, weight);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%" PRIu64 ":%" PRIu64, res.reached, res.total,
             res.max);
    free(to);
    free(weight);
    return true;
}
