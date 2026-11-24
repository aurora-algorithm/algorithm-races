// Race: spectral-norm (c)
// Extracted from suite/c/src/bg.c — build and run via suite/run.py

/* Benchmarks Game kernels, single-threaded (SPEC.md section 4). Floating-point code keeps the
 * operation order of the spec so every language prints the same digits. */
#include "bg.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernels.h"

static double eval_a(size_t i, size_t j) {
    return 1.0 / (double)((i + j) * (i + j + 1) / 2 + i + 1);
}

static void mul_av(const double *u, double *v, size_t n) {
    for (size_t i = 0; i < n; i++) {
        double s = 0.0;
        for (size_t j = 0; j < n; j++) {
            s += eval_a(i, j) * u[j];
        }
        v[i] = s;
    }
}

static void mul_atv(const double *u, double *v, size_t n) {
    for (size_t i = 0; i < n; i++) {
        double s = 0.0;
        for (size_t j = 0; j < n; j++) {
            s += eval_a(j, i) * u[j];
        }
        v[i] = s;
    }
}

static void mul_atav(const double *u, double *v, double *tmp, size_t n) {
    mul_av(u, tmp, n);
    mul_atv(tmp, v, n);
}

static double spectral_norm_run(size_t n) {
    double *u = xmalloc(n * sizeof *u);
    double *v = xmalloc(n * sizeof *v);
    double *tmp = xmalloc(n * sizeof *tmp);
    for (size_t i = 0; i < n; i++) {
        u[i] = 1.0;
    }
    for (int i = 0; i < 10; i++) {
        mul_atav(u, v, tmp, n);
        mul_atav(v, u, tmp, n);
    }
    double vbv = 0.0, vv = 0.0;
    for (size_t i = 0; i < n; i++) {
        vbv += u[i] * v[i];
        vv += v[i] * v[i];
    }
    free(u);
    free(v);
    free(tmp);
    return sqrt(vbv / vv);
}

static bool spectral_norm(const Args *args, Result *out) {
    double result = 0.0;
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        result = spectral_norm_run(args->size);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%.9f", result);
    return true;
}
