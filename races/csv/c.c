// Race: csv (c)
// Extracted from suite/c/src/work_files.c — build and run via suite/run.py

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
