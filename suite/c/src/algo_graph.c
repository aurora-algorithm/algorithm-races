/* Graph kernels of group `algo` (SPEC.md section 12): bfs-grid, dijkstra, kruskal. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernels.h"

static bool *grid(size_t n) {
    Rng rng = rng_new();
    bool *walls = xmalloc(n * n * sizeof *walls);
    for (size_t i = 0; i < n * n; i++) {
        walls[i] = rng_next(&rng) % 100 < 30;
    }
    const size_t open_cells[6] = {0, 1, n, n * n - n - 1, n * n - 2, n * n - 1};
    for (int i = 0; i < 6; i++) {
        walls[open_cells[i]] = false;
    }
    return walls;
}

/* BFS from cell 0; the queue is an array with a head index (each cell enters it at most once). */
typedef struct {
    size_t reached;
    int32_t last;
    int64_t total;
} BfsResult;

static BfsResult bfs_grid_run(size_t n, const bool *walls) {
    int32_t *dist = xmalloc(n * n * sizeof *dist);
    for (size_t i = 0; i < n * n; i++) {
        dist[i] = -1;
    }
    size_t *queue = xmalloc(n * n * sizeof *queue);
    size_t tail = 0;
    dist[0] = 0;
    queue[tail++] = 0;
    for (size_t head = 0; head < tail; head++) {
        size_t cell = queue[head];
        int32_t d = dist[cell] + 1;
        size_t row = cell / n, col = cell % n;
        const struct {
            size_t next;
            bool ok;
        } neighbours[4] = {{cell - n, row > 0},
                           {cell + n, row < n - 1},
                           {cell - 1, col > 0},
                           {cell + 1, col < n - 1}};
        for (int k = 0; k < 4; k++) {
            size_t next = neighbours[k].next;
            if (neighbours[k].ok && !walls[next] && dist[next] < 0) {
                dist[next] = d;
                queue[tail++] = next;
            }
        }
    }
    size_t reached = 0;
    int64_t total = 0;
    for (size_t i = 0; i < n * n; i++) {
        if (dist[i] >= 0) {
            reached++;
            total += dist[i];
        }
    }
    BfsResult result = {reached, dist[n * n - 1], total};
    free(queue);
    free(dist);
    return result;
}

static bool bfs_grid(const Args *args, Result *out) {
    bool *walls = grid(args->size);
    BfsResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = bfs_grid_run(args->size, walls);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%" PRId32 ":%" PRId64, res.reached, res.last,
             res.total);
    free(walls);
    return true;
}

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

const Kernel GRAPH_KERNELS[] = {
    {"bfs-grid", bfs_grid}, {"dijkstra", dijkstra}, {"kruskal", kruskal}, {NULL, NULL}};
