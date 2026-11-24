// Race: kruskal (c)
// Extracted from suite/c/src/algo_graph.c — build and run via suite/run.py

/* Graph kernels of group `algo` (SPEC.md section 12): bfs-grid, dijkstra, kruskal. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

typedef struct {
    uint32_t w;
    size_t u, v;
} Edge;

static Edge *edge_list(size_t n) {
    Rng rng = rng_new();
    Edge *edges = xmalloc(4 * n * sizeof *edges);
    for (size_t i = 0; i < 4 * n; i++) {
        size_t u = rng_next(&rng) % n;
        size_t v = rng_next(&rng) % n;
        edges[i] = (Edge){rng_next(&rng) % 1000000, u, v};
    }
    return edges;
}

static int compare_weight(const void *pa, const void *pb) {
    uint32_t a = ((const Edge *)pa)->w;
    uint32_t b = ((const Edge *)pb)->w;
    return (a > b) - (a < b);
}

/* Find with full path compression, two passes, no recursion. */
static size_t find(size_t *parent, size_t x) {
    size_t root = x;
    while (parent[root] != root) {
        root = parent[root];
    }
    while (parent[x] != root) {
        size_t next = parent[x];
        parent[x] = root;
        x = next;
    }
    return root;
}

typedef struct {
    uint64_t count, total;
} Forest;

static Forest kruskal_run(size_t n, Edge *edges) {
    size_t m = 4 * n;
    qsort(edges, m, sizeof *edges, compare_weight);
    size_t *parent = xmalloc(n * sizeof *parent);
    unsigned char *rank = xcalloc(n, 1);
    for (size_t i = 0; i < n; i++) {
        parent[i] = i;
    }
    uint64_t count = 0, total = 0;
    for (size_t i = 0; i < m; i++) {
        size_t ru = find(parent, edges[i].u);
        size_t rv = find(parent, edges[i].v);
        if (ru == rv) {
            continue;
        }
        if (rank[ru] < rank[rv]) {
            size_t t = ru;
            ru = rv;
            rv = t;
        }
        parent[rv] = ru;
        if (rank[ru] == rank[rv]) {
            rank[ru]++;
        }
        count++;
        total += edges[i].w;
    }
    free(rank);
    free(parent);
    return (Forest){count, total};
}

static bool kruskal(const Args *args, Result *out) {
    size_t m = 4 * args->size;
    Edge *input = edge_list(args->size);
    Edge *edges = xmalloc(m * sizeof *edges);
    Forest res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        memcpy(edges, input, m * sizeof *edges);
        double start = now_ms();
        res = kruskal_run(args->size, edges);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu64 ":%" PRIu64, res.count, res.total);
    free(edges);
    free(input);
    return true;
}
