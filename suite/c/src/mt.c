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

typedef struct {
    size_t n, first, step;
    unsigned char *bitmap;
} RowJob;

static void mandelbrot_rows(void *p) {
    RowJob *job = p;
    for (size_t y = job->first; y < job->n; y += job->step) {
        mandelbrot_row(job->n, y, job->bitmap + y * row_bytes(job->n));
    }
}

/* Thread t computes rows t, t + T, t + 2T, ... of the shared bitmap. */
static bool mandelbrot_mt(const Args *args, Result *out) {
    size_t n = args->size;
    size_t threads = args->threads;
    size_t bytes = n * row_bytes(n);
    Thread *workers = xmalloc(threads * sizeof *workers);
    RowJob *jobs = xmalloc(threads * sizeof *jobs);
    unsigned char *bitmap = NULL;
    for (size_t r = 0; r < args->repeats; r++) {
        free(bitmap);
        double start = now_ms();
        bitmap = xcalloc(bytes, 1);
        for (size_t t = 0; t < threads; t++) {
            jobs[t] = (RowJob){n, t, threads, bitmap};
            thread_start(&workers[t], mandelbrot_rows, &jobs[t]);
        }
        for (size_t t = 0; t < threads; t++) {
            thread_join(&workers[t]);
        }
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu32, fnv1a(bitmap, bytes));
    free(bitmap);
    free(jobs);
    free(workers);
    return true;
}

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

const Kernel MT_KERNELS[] = {
    {"mergesort-mt", sort_mt},
    {"mandelbrot-mt", mandelbrot_mt},
    {"binary-trees-mt", binary_trees_mt},
    {NULL, NULL},
};
