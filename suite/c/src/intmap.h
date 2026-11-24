/* A hand-written hash map from int64 to int64 (C has none in its standard library): open
 * addressing, linear probing, a power-of-two table that doubles at 3/4 load, and MurmurHash3's
 * 64-bit finalizer. */
#ifndef KERNELS_INTMAP_H
#define KERNELS_INTMAP_H

#include "common.h"

typedef struct {
    int64_t key;
    int64_t value;
    bool used;
} IntMapSlot;

typedef struct {
    IntMapSlot *slots;
    size_t cap;
    size_t len;
} IntMap;

/* An empty map of 16 slots. */
IntMap intmap_new(void);
/* An empty map that holds `keys` keys without growing. */
IntMap intmap_with_capacity(size_t keys);
void intmap_free(IntMap *m);
/* The value of key, inserted as 0 when absent. */
int64_t *intmap_entry(IntMap *m, int64_t key);
bool intmap_contains(const IntMap *m, int64_t key);
/* The value of key, or NULL when absent. */
int64_t *intmap_get(const IntMap *m, int64_t key);
/* Removes key if present (backward-shift deletion, no tombstones). */
void intmap_remove(IntMap *m, int64_t key);

#endif
