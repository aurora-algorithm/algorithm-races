/* Work kernels csv (libcsv) and wordcount (stdio lines, our own string hash map), SPEC.md
 * section 11. Both read their file inside the timed region. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <csv.h>

#include "intmap.h"
#include "kernels.h"

#define READ_CHUNK (64 * 1024)

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

static uint64_t parse_u64(const char *s, size_t len) {
    uint64_t v = 0;
    for (size_t i = 0; i < len; i++) {
        v = v * 10 + (uint64_t)(s[i] - '0');
    }
    return v;
}

/* libcsv calls on_field per field and on_record at each record's end. */
typedef struct {
    size_t field;
    bool header;
    long customer_col, total_col;
    uint64_t customer, total;
    uint64_t rows;
    IntMap totals; /* customer_id -> sum of total_cents */
} CsvState;

static void on_field(void *data, size_t len, void *ctx) {
    CsvState *st = ctx;
    const char *s = data;
    if (st->header) {
        if (len == 11 && memcmp(s, "customer_id", 11) == 0) {
            st->customer_col = (long)st->field;
        } else if (len == 11 && memcmp(s, "total_cents", 11) == 0) {
            st->total_col = (long)st->field;
        }
    } else if ((long)st->field == st->customer_col) {
        st->customer = parse_u64(s, len);
    } else if ((long)st->field == st->total_col) {
        st->total = parse_u64(s, len);
    }
    st->field++;
}

static void on_record(int terminator, void *ctx) {
    (void)terminator;
    CsvState *st = ctx;
    if (st->header) {
        st->header = false;
    } else {
        st->rows++;
        *intmap_entry(&st->totals, (int64_t)st->customer) += (int64_t)st->total;
    }
    st->field = 0;
}

static bool csv_totals(const Args *args, CsvState *st) {
    FILE *f = open_data(args, "orders-%zu.csv");
    if (f == NULL) {
        return false;
    }
    *st = (CsvState){.header = true, .customer_col = -1, .total_col = -1, .totals = intmap_new()};
    struct csv_parser parser;
    csv_init(&parser, 0);
    char *buf = xmalloc(READ_CHUNK);
    size_t got;
    bool ok = true;
    while (ok && (got = fread(buf, 1, READ_CHUNK, f)) > 0) {
        ok = csv_parse(&parser, buf, got, on_field, on_record, st) == got;
    }
    ok = ok && !ferror(f) && csv_fini(&parser, on_field, on_record, st) == 0;
    if (!ok) {
        fprintf(stderr, "error: csv: %s\n", csv_strerror(csv_error(&parser)));
    } else if (st->customer_col < 0 || st->total_col < 0) {
        fprintf(stderr, "error: csv: header without customer_id and total_cents\n");
        ok = false;
    }
    csv_free(&parser);
    free(buf);
    fclose(f);
    return ok;
}

static bool csv(const Args *args, Result *out) {
    CsvState st = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        intmap_free(&st.totals);
        double start = now_ms();
        bool ok = csv_totals(args, &st);
        out->times[r] = now_ms() - start;
        if (!ok) {
            intmap_free(&st.totals);
            return false;
        }
    }
    uint64_t sum = 0, top = 0;
    int64_t top_id = 0;
    for (size_t i = 0; i < st.totals.cap; i++) {
        const IntMapSlot *s = &st.totals.slots[i];
        if (!s->used) {
            continue;
        }
        uint64_t v = (uint64_t)s->value;
        sum += v;
        if (v > top || (v == top && s->key < top_id)) {
            top = v;
            top_id = s->key;
        }
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu64 ":%zu:%" PRIu64 ":%" PRId64 ":%" PRIu64,
             st.rows, st.totals.len, sum, top_id, top);
    intmap_free(&st.totals);
    return true;
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

const Kernel FILE_KERNELS[] = {{"csv", csv}, {"wordcount", wordcount}, {NULL, NULL}};
