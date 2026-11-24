// Race: wordcount (c)
// Extracted from suite/c/src/work_files.c — build and run via suite/run.py

/* Work kernels csv (libcsv) and wordcount (stdio lines, our own string hash map), SPEC.md
 * section 11. Both read their file inside the timed region. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <csv.h>
#include "intmap.h"
#include "kernels.h"

static FILE *open_data(const Args *args, const char *fmt) {
    char name[64];
    snprintf(name, sizeof name, fmt, args->size);
    char *path = path_join(args->data, name);
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "error: cannot open %s\n", path);
    }
    free(path);
    return f;
}

/* A hash map from words to counts: open addressing, linear probing, FNV-1a, keys copied with
 * malloc. */
typedef struct {
    char *key; /* NULL: empty slot */
    size_t len;
    uint32_t hash;
    uint64_t count;
} WordSlot;

typedef struct {
    WordSlot *slots;
    size_t cap, len;
} WordMap;

static size_t word_slot(const WordMap *m, const char *w, size_t len, uint32_t hash) {
    size_t mask = m->cap - 1;
    size_t i = hash & mask;
    while (m->slots[i].key != NULL && !(m->slots[i].hash == hash && m->slots[i].len == len &&
                                        memcmp(m->slots[i].key, w, len) == 0)) {
        i = (i + 1) & mask;
    }
    return i;
}

static void word_grow(WordMap *m) {
    WordMap bigger = {xcalloc(m->cap * 2, sizeof(WordSlot)), m->cap * 2, m->len};
    for (size_t i = 0; i < m->cap; i++) {
        WordSlot *s = &m->slots[i];
        if (s->key != NULL) {
            size_t mask = bigger.cap - 1;
            size_t j = s->hash & mask;
            while (bigger.slots[j].key != NULL) {
                j = (j + 1) & mask;
            }
            bigger.slots[j] = *s;
        }
    }
    free(m->slots);
    *m = bigger;
}

static void word_add(WordMap *m, const char *w, size_t len) {
    uint32_t hash = fnv1a(w, len);
    size_t i = word_slot(m, w, len, hash);
    if (m->slots[i].key != NULL) {
        m->slots[i].count++;
        return;
    }
    if ((m->len + 1) * 4 > m->cap * 3) {
        word_grow(m);
        i = word_slot(m, w, len, hash);
    }
    char *key = xmalloc(len);
    memcpy(key, w, len);
    m->slots[i] = (WordSlot){key, len, hash, 1};
    m->len++;
}

static void word_free(WordMap *m) {
    for (size_t i = 0; i < m->cap; i++) {
        free(m->slots[i].key);
    }
    free(m->slots);
    *m = (WordMap){0};
}

static bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
}

/* Reads one line (any length) into *line; false at the end of the file. */
static bool read_line(FILE *f, char **line, size_t *cap, size_t *len) {
    *len = 0;
    for (;;) {
        if (*cap - *len < 2) {
            *cap *= 2;
            *line = xrealloc(*line, *cap);
        }
        if (fgets(*line + *len, (int)(*cap - *len), f) == NULL) {
            return *len > 0;
        }
        *len += strlen(*line + *len);
        if ((*line)[*len - 1] == '\n') {
            return true;
        }
    }
}

static bool wordcount_run(const Args *args, WordMap *counts) {
    FILE *f = open_data(args, "text-%zu.txt");
    if (f == NULL) {
        return false;
    }
    *counts = (WordMap){xcalloc(16, sizeof(WordSlot)), 16, 0};
    size_t cap = 256, len;
    char *line = xmalloc(cap);
    while (read_line(f, &line, &cap, &len)) {
        size_t i = 0;
        while (i < len) {
            while (i < len && is_space(line[i])) {
                i++;
            }
            size_t start = i;
            while (i < len && !is_space(line[i])) {
                i++;
            }
            if (i > start) {
                word_add(counts, line + start, i - start);
            }
        }
    }
    bool ok = !ferror(f);
    free(line);
    fclose(f);
    if (!ok) {
        fprintf(stderr, "error: reading text-%zu.txt failed\n", args->size);
    }
    return ok;
}

/* Whether a comes before b in the checksum's order: the larger count, then the smaller word. */
static bool word_before(const WordSlot *a, const WordSlot *b) {
    if (a->count != b->count) {
        return a->count > b->count;
    }
    size_t n = a->len < b->len ? a->len : b->len;
    int c = memcmp(a->key, b->key, n);
    return c < 0 || (c == 0 && a->len < b->len);
}

static bool wordcount(const Args *args, Result *out) {
    WordMap counts = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        word_free(&counts);
        double start = now_ms();
        bool ok = wordcount_run(args, &counts);
        out->times[r] = now_ms() - start;
        if (!ok) {
            word_free(&counts);
            return false;
        }
    }
    uint64_t total = 0;
    const WordSlot *top = NULL;
    for (size_t i = 0; i < counts.cap; i++) {
        const WordSlot *s = &counts.slots[i];
        if (s->key != NULL) {
            total += s->count;
            if (top == NULL || word_before(s, top)) {
                top = s;
            }
        }
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu64 ":%zu:%.*s:%" PRIu64, total, counts.len,
             top ? (int)top->len : 0, top ? top->key : "", top ? top->count : 0);
    word_free(&counts);
    return true;
}
