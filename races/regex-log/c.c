// Race: regex-log (c)
// Extracted from suite/c/src/lib.c — build and run via suite/run.py

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
