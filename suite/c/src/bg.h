#ifndef KERNELS_BG_H
#define KERNELS_BG_H

#include "common.h"

#define MIN_DEPTH 4u

/* Builds and checks `iterations` trees of `depth`, freeing each; returns the sum of the checks. */
uint64_t depth_checks(unsigned depth, uint64_t iterations);

/* Runs the trees of one depth; `ctx` is the caller's (the thread count for the -mt kernel). */
typedef uint64_t (*DepthRun)(unsigned depth, uint64_t iterations, const void *ctx);

/* The Benchmarks Game output text of binary-trees N into `out`. */
void binary_trees_text(unsigned n, DepthRun run, const void *ctx, Bytes *out);

/* binary-trees and binary-trees-mt: the text per repeat, checksum fnv1a of the last. */
bool run_binary_trees(const Args *args, Result *out, DepthRun run);

size_t row_bytes(size_t n);
/* One bitmap row: bit x (MSB first) is set when point (x, y) stays in the set for 50 iterations. */
void mandelbrot_row(size_t n, size_t y, unsigned char *row);

#endif
