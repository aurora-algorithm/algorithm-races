#include "intmap.h"

#include <stdlib.h>

#define INITIAL_SLOTS 16

static uint64_t hash64(int64_t key) {
    uint64_t h = (uint64_t)key;
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdu;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53u;
    h ^= h >> 33;
    return h;
}

IntMap intmap_new(void) {
    return (IntMap){xcalloc(INITIAL_SLOTS, sizeof(IntMapSlot)), INITIAL_SLOTS, 0};
}

IntMap intmap_with_capacity(size_t keys) {
    size_t cap = INITIAL_SLOTS;
    while (keys * 4 > cap * 3) {
        cap *= 2;
    }
    return (IntMap){xcalloc(cap, sizeof(IntMapSlot)), cap, 0};
}

void intmap_free(IntMap *m) {
    free(m->slots);
    *m = (IntMap){0};
}

static size_t find_slot(const IntMapSlot *slots, size_t cap, int64_t key) {
    size_t mask = cap - 1;
    size_t i = (size_t)hash64(key) & mask;
    while (slots[i].used && slots[i].key != key) {
        i = (i + 1) & mask;
    }
    return i;
}

static void grow(IntMap *m) {
    size_t cap = m->cap * 2;
    IntMapSlot *slots = xcalloc(cap, sizeof *slots);
    for (size_t i = 0; i < m->cap; i++) {
        if (m->slots[i].used) {
            slots[find_slot(slots, cap, m->slots[i].key)] = m->slots[i];
        }
    }
    free(m->slots);
    m->slots = slots;
    m->cap = cap;
}

int64_t *intmap_entry(IntMap *m, int64_t key) {
    size_t i = find_slot(m->slots, m->cap, key);
    if (m->slots[i].used) {
        return &m->slots[i].value;
    }
    if ((m->len + 1) * 4 > m->cap * 3) {
        grow(m);
        i = find_slot(m->slots, m->cap, key);
    }
    m->slots[i] = (IntMapSlot){key, 0, true};
    m->len++;
    return &m->slots[i].value;
}

bool intmap_contains(const IntMap *m, int64_t key) {
    return m->slots[find_slot(m->slots, m->cap, key)].used;
}

int64_t *intmap_get(const IntMap *m, int64_t key) {
    size_t i = find_slot(m->slots, m->cap, key);
    return m->slots[i].used ? &m->slots[i].value : NULL;
}

void intmap_remove(IntMap *m, int64_t key) {
    size_t mask = m->cap - 1;
    size_t hole = find_slot(m->slots, m->cap, key);
    if (!m->slots[hole].used) {
        return;
    }
    m->slots[hole].used = false;
    m->len--;
    /* Moves back every later key of the run whose home slot is not between the hole and itself. */
    for (size_t i = (hole + 1) & mask; m->slots[i].used; i = (i + 1) & mask) {
        size_t home = (size_t)hash64(m->slots[i].key) & mask;
        if (((i - home) & mask) >= ((i - hole) & mask)) {
            m->slots[hole] = m->slots[i];
            m->slots[i].used = false;
            hole = i;
        }
    }
}
