// Race: binary-trees-mt (c)
// Extracted from suite/c/src/mt.c — build and run via suite/run.py

/* Multi-threaded kernels (SPEC.md section 6). Threads start inside the timed call, as Rust's scoped
 * threads do: starting an OS thread costs microseconds, not the millisecond that would call for a
 * pool. */
#include <stdio.h>
#include <stdlib.h>
#include "bg.h"
#include "kernels.h"
#include "sorts.h"
#include "thread.h"

typedef struct {
    unsigned depth;
    uint64_t iterations;
    uint64_t check;
} TreeJob;

static void tree_share(void *p) {
    TreeJob *job = p;
    job->check = depth_checks(job->depth, job->iterations);
}

/* Each depth's iterations are split into T equal shares; the main thread sums the checks. */
static uint64_t parallel_depth(unsigned depth, uint64_t iterations, const void *ctx) {
    size_t threads = *(const size_t *)ctx;
    Thread *workers = xmalloc(threads * sizeof *workers);
    TreeJob *jobs = xmalloc(threads * sizeof *jobs);
    for (size_t t = 0; t < threads; t++) {
        jobs[t] = (TreeJob){depth, iterations / threads, 0};
        thread_start(&workers[t], tree_share, &jobs[t]);
    }
    uint64_t sum = 0;
    for (size_t t = 0; t < threads; t++) {
        thread_join(&workers[t]);
        sum += jobs[t].check;
    }
    free(jobs);
    free(workers);
    return sum;
}

static bool binary_trees_mt(const Args *args, Result *out) {
    return run_binary_trees(args, out, parallel_depth);
}
