/* Library kernels (SPEC.md section 5): regex-log with PCRE2, hashmap with our own table (C has no
 * standard one), sha256 with Mbed TLS, json with cJSON. */
#define PCRE2_CODE_UNIT_WIDTH 8

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON.h>
#include <mbedtls/sha256.h>
#include <pcre2.h>

#include "intmap.h"
#include "kernels.h"

#define LOG_PATTERN "(GET|POST) /api/v[0-9]/users/([0-9]+) (5[0-9][0-9])"

static const char *const METHODS[4] = {"GET", "POST", "PUT", "DELETE"};
static const unsigned STATUSES[12] = {200, 201, 204, 301, 304, 400, 401, 403, 404, 500, 502, 503};

static void log_text(size_t lines, Bytes *text) {
    Rng rng = rng_new();
    for (size_t i = 0; i < lines; i++) {
        const char *method = METHODS[rng_next(&rng) % 4];
        uint32_t version = 1 + rng_next(&rng) % 3;
        uint32_t user = rng_next(&rng) % 100000;
        unsigned status = STATUSES[rng_next(&rng) % 12];
        uint32_t ms = rng_next(&rng) % 1000;
        bytes_printf(text, "%s /api/v%" PRIu32 "/users/%" PRIu32 " %u %" PRIu32 "ms\n", method,
                     version, user, status, ms);
    }
}

static uint64_t parse_digits(const unsigned char *s, size_t len) {
    uint64_t v = 0;
    for (size_t i = 0; i < len; i++) {
        v = v * 10 + (uint64_t)(s[i] - '0');
    }
    return v;
}

static bool regex_log(const Args *args, Result *out) {
    Bytes text = {0};
    log_text(args->size, &text);
    int error;
    PCRE2_SIZE error_offset;
    pcre2_code *re = pcre2_compile((PCRE2_SPTR)LOG_PATTERN, PCRE2_ZERO_TERMINATED, 0, &error,
                                   &error_offset, NULL);
    if (re == NULL) {
        PCRE2_UCHAR message[256];
        pcre2_get_error_message(error, message, sizeof message);
        fprintf(stderr, "error: pattern at %zu: %s\n", (size_t)error_offset, (char *)message);
        free(text.data);
        return false;
    }
    uint64_t matches = 0, ids = 0;
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        pcre2_match_data *md = pcre2_match_data_create_from_pattern(re, NULL);
        matches = ids = 0;
        PCRE2_SIZE offset = 0;
        while (pcre2_match(re, text.data, text.len, offset, 0, md, NULL) > 0) {
            PCRE2_SIZE *ov = pcre2_get_ovector_pointer(md);
            matches++;
            ids += parse_digits(text.data + ov[4], ov[5] - ov[4]);
            offset = ov[1];
        }
        pcre2_match_data_free(md);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu64 ":%" PRIu64, matches, ids);
    pcre2_code_free(re);
    free(text.data);
    return true;
}

static bool hashmap(const Args *args, Result *out) {
    size_t n = args->size;
    int64_t *inserts, *lookups;
    hashmap_input(n, &inserts, &lookups);
    size_t keys = 0, hits = 0;
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        IntMap counts = intmap_new();
        for (size_t i = 0; i < n; i++) {
            *intmap_entry(&counts, inserts[i]) += 1;
        }
        hits = 0;
        for (size_t i = 0; i < n; i++) {
            hits += intmap_contains(&counts, lookups[i]);
        }
        keys = counts.len;
        intmap_free(&counts);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%zu", keys, hits);
    free(inserts);
    free(lookups);
    return true;
}

/* One 1 MiB block of generator values (4 bytes each, little-endian), repeated to `size` bytes. */
static unsigned char *sha_buffer(size_t size) {
    const size_t block = (size_t)1 << 20;
    unsigned char *buf = xmalloc(size);
    Rng rng = rng_new();
    for (size_t i = 0; i < block && i < size; i += 4) {
        uint32_t v = rng_next(&rng);
        for (size_t b = 0; b < 4; b++) {
            buf[i + b] = (unsigned char)(v >> (8 * b));
        }
    }
    for (size_t off = block; off + block <= size; off += block) {
        memcpy(buf + off, buf, block);
    }
    return buf;
}

static bool sha256(const Args *args, Result *out) {
    size_t n = args->size;
    unsigned char *buf = sha_buffer(n);
    unsigned char digest[32];
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        int rc = mbedtls_sha256(buf, n, digest, 0);
        out->times[r] = now_ms() - start;
        if (rc != 0) {
            fprintf(stderr, "error: mbedtls_sha256 returned %d\n", rc);
            free(buf);
            return false;
        }
    }
    for (int i = 0; i < 32; i++) {
        snprintf(out->checksum + 2 * i, 3, "%02x", digest[i]);
    }
    free(buf);
    return true;
}

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

const Kernel LIB_KERNELS[] = {
    {"regex-log", regex_log}, {"hashmap", hashmap}, {"sha256", sha256},
    {"json", json},           {NULL, NULL},
};
