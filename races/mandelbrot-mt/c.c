// Race: mandelbrot-mt (c)
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
