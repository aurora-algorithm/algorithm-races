// Race: bfs-grid (c)
// Extracted from suite/c/src/algo_graph.c — build and run via suite/run.py

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
