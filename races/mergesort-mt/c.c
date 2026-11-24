// Race: mergesort-mt (c)
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
    int64_t *a;
    int64_t *aux;
    size_t lo, mid, hi;
} MergeJob;

static void sort_chunk(void *p) {
    MergeJob *job = p;
    merge_sort(job->a, job->aux, job->lo, job->hi);
}

static void merge_runs(void *p) {
    MergeJob *job = p;
    merge(job->a, job->aux, job->lo, job->mid, job->hi);
}

/* T threads each mergesort one chunk, then rounds of pairwise merges of neighbouring runs (one
 * thread per merge) until one run is left. `threads` is a power of two. */
static void mergesort_mt(int64_t *a, size_t n, size_t threads) {
    int64_t *aux = xmalloc(n * sizeof *aux);
    size_t *bounds = xmalloc((threads + 1) * sizeof *bounds);
    for (size_t t = 0; t <= threads; t++) {
        bounds[t] = t * n / threads;
    }
    Thread *workers = xmalloc(threads * sizeof *workers);
    MergeJob *jobs = xmalloc(threads * sizeof *jobs);
    for (size_t t = 0; t < threads; t++) {
        jobs[t] = (MergeJob){a, aux, bounds[t], 0, bounds[t + 1]};
        thread_start(&workers[t], sort_chunk, &jobs[t]);
    }
    for (size_t t = 0; t < threads; t++) {
        thread_join(&workers[t]);
    }
    for (size_t runs = threads; runs > 1; runs /= 2) {
        size_t pairs = runs / 2;
        for (size_t p = 0; p < pairs; p++) {
            jobs[p] = (MergeJob){a, aux, bounds[2 * p], bounds[2 * p + 1], bounds[2 * p + 2]};
            thread_start(&workers[p], merge_runs, &jobs[p]);
        }
        for (size_t p = 0; p < pairs; p++) {
            thread_join(&workers[p]);
        }
        for (size_t p = 0; p <= pairs; p++) {
            bounds[p] = bounds[2 * p];
        }
    }
    free(jobs);
    free(workers);
    free(bounds);
    free(aux);
}

static bool is_power_of_two(size_t v) {
    return v != 0 && (v & (v - 1)) == 0;
}

static bool sort_mt(const Args *args, Result *out) {
    if (!is_power_of_two(args->threads)) {
        fprintf(stderr, "error: --threads must be a power of two\n");
        return false;
    }
    return run_sort(args, out, mergesort_mt);
}
