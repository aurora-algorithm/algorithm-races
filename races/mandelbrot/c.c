// Race: mandelbrot (c)
// Extracted from suite/c/src/bg.c — build and run via suite/run.py

/* Benchmarks Game kernels, single-threaded (SPEC.md section 4). Floating-point code keeps the
 * operation order of the spec so every language prints the same digits. */
#include "bg.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

size_t row_bytes(size_t n) {
    return (n + 7) / 8;
}

void mandelbrot_row(size_t n, size_t y, unsigned char *row) {
    double ci = 2.0 * (double)y / (double)n - 1.0;
    for (size_t x = 0; x < n; x++) {
        double cr = 2.0 * (double)x / (double)n - 1.5;
        double zr = 0.0, zi = 0.0, tr = 0.0, ti = 0.0;
        for (int i = 0; i < 50 && tr + ti <= 4.0; i++) {
            zi = 2.0 * zr * zi + ci;
            zr = tr - ti + cr;
            tr = zr * zr;
            ti = zi * zi;
        }
        if (tr + ti <= 4.0) {
            row[x >> 3] |= (unsigned char)(0x80u >> (x & 7));
        }
    }
}

static bool mandelbrot(const Args *args, Result *out) {
    size_t n = args->size;
    size_t bytes = n * row_bytes(n);
    unsigned char *bitmap = NULL;
    for (size_t r = 0; r < args->repeats; r++) {
        free(bitmap);
        double start = now_ms();
        bitmap = xcalloc(bytes, 1);
        for (size_t y = 0; y < n; y++) {
            mandelbrot_row(n, y, bitmap + y * row_bytes(n));
        }
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu32, fnv1a(bitmap, bytes));
    free(bitmap);
    return true;
}
