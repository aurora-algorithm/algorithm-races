/* The kernel tables of every module; main.c looks a kernel up by name in these. */
#ifndef KERNELS_KERNELS_H
#define KERNELS_KERNELS_H

#include "common.h"

extern const Kernel SORT_KERNELS[];
extern const Kernel BG_KERNELS[];
extern const Kernel MT_KERNELS[];
extern const Kernel LIB_KERNELS[];
extern const Kernel LOOP_KERNELS[];
extern const Kernel GRAPH_KERNELS[];
extern const Kernel TREE_KERNELS[];
extern const Kernel DP_KERNELS[];
extern const Kernel PUZZLE_KERNELS[];
extern const Kernel SUDOKU_KERNELS[];
extern const Kernel TEXT_KERNELS[];
extern const Kernel COMPRESS_KERNELS[];
extern const Kernel PASSWORD_KERNELS[];
extern const Kernel FILE_KERNELS[];
extern const Kernel IMAGE_KERNELS[];
extern const Kernel ARENA_KERNELS[];
extern const Kernel WAYS_KERNELS[];
extern const Kernel WORDS_KERNELS[];
extern const Kernel SSEARCH_KERNELS[];
extern const Kernel AUTOCOMPLETE_KERNELS[];

/* The libraries a kernel calls, for the JSON line's `runtime` field; NULL when it uses none. */
const char *kernel_libraries(const char *name);

#endif
