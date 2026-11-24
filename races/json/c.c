// Race: json (c)
// Extracted from suite/c/src/lib.c — build and run via suite/run.py

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>
#include <mbedtls/sha256.h>
#include <pcre2.h>
#include "intmap.h"
#include "kernels.h"

/* Parses the whole document, sums two fields of every order, writes it back as compact JSON. */
static char *json_roundtrip(const char *text, size_t *orders, uint64_t *cents, uint64_t *items) {
    cJSON *doc = cJSON_Parse(text);
    const cJSON *list = cJSON_GetObjectItemCaseSensitive(doc, "orders");
    if (!cJSON_IsArray(list)) {
        cJSON_Delete(doc);
        return NULL;
    }
    *orders = 0;
    *cents = *items = 0;
    const cJSON *order;
    cJSON_ArrayForEach(order, list) {
        *cents += (uint64_t)cJSON_GetObjectItemCaseSensitive(order, "total_cents")->valuedouble;
        *items += (uint64_t)cJSON_GetObjectItemCaseSensitive(order, "items")->valuedouble;
        (*orders)++;
    }
    char *output = cJSON_PrintUnformatted(doc);
    cJSON_Delete(doc);
    return output;
}

static bool json(const Args *args, Result *out) {
    char name[64];
    snprintf(name, sizeof name, "orders-%zu.json", args->size);
    char *path = path_join(args->data, name);
    size_t len;
    unsigned char *raw = read_file(path, &len);
    free(path);
    if (raw == NULL) {
        return false;
    }
    char *text = xrealloc(raw, len + 1);
    text[len] = '\0';
    char *output = NULL;
    size_t orders = 0;
    uint64_t cents = 0, items = 0;
    for (size_t r = 0; r < args->repeats; r++) {
        free(output);
        double start = now_ms();
        output = json_roundtrip(text, &orders, &cents, &items);
        out->times[r] = now_ms() - start;
        if (output == NULL) {
            fprintf(stderr, "error: orders-%zu.json is not an orders document\n", args->size);
            free(text);
            return false;
        }
    }
    size_t out_len = strlen(output);
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%" PRIu64 ":%" PRIu64 ":%zu:%" PRIu32, orders, cents,
             items, out_len, fnv1a(output, out_len));
    free(output);
    free(text);
    return true;
}
