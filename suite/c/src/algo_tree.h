#ifndef KERNELS_ALGO_TREE_H
#define KERNELS_ALGO_TREE_H

#include "common.h"

/* N words (3..10 letters) then N prefixes (1..4 letters), each NUL-terminated in one block. */
typedef struct {
    char **words;
    char **prefixes;
    char *block;
} Words;

/* The trie input of SPEC.md section 12; free the three arrays when done. */
Words words(size_t n);

#endif
