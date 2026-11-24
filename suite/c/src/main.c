/* CPU kernels, C version (see ../../SPEC.md).
 *
 * usage: kernels KERNEL --size N [--repeats R] [--threads T] [--data DIR]
 * Prints one JSON line: kernel, size, threads, checksum, times_ms (kernel only), runtime. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_info.h"
#include "kernels.h"

#define STR_(x) #x
#define STR(x) STR_(x)

#if defined(__clang__)
#define COMPILER "clang " __clang_version__
#elif defined(__GNUC__)
#define COMPILER "gcc " __VERSION__
#elif defined(_MSC_VER)
#define COMPILER "MSVC " STR(_MSC_FULL_VER)
#else
#define COMPILER "unknown compiler"
#endif

static const Kernel *const TABLES[] = {
    SORT_KERNELS,  BG_KERNELS,       MT_KERNELS,       LIB_KERNELS,          LOOP_KERNELS,
    GRAPH_KERNELS, TREE_KERNELS,     DP_KERNELS,       PUZZLE_KERNELS,       SUDOKU_KERNELS,
    TEXT_KERNELS,  COMPRESS_KERNELS, PASSWORD_KERNELS, FILE_KERNELS,         IMAGE_KERNELS,
    ARENA_KERNELS, WAYS_KERNELS,     SSEARCH_KERNELS,  AUTOCOMPLETE_KERNELS, WORDS_KERNELS,
};

static const Kernel *find_kernel(const char *name) {
    for (size_t t = 0; t < sizeof TABLES / sizeof TABLES[0]; t++) {
        for (const Kernel *k = TABLES[t]; k->name != NULL; k++) {
            if (strcmp(k->name, name) == 0) {
                return k;
            }
        }
    }
    return NULL;
}

static bool parse_count(const char *flag, const char *text, size_t *out) {
    char *end;
    errno = 0;
    unsigned long long v = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || v == 0 || text[0] == '-') {
        fprintf(stderr, "error: %s: expected a positive integer, got %s\n", flag, text);
        return false;
    }
    *out = (size_t)v;
    return true;
}

static bool parse_args(int argc, char **argv, Args *args) {
    if (argc < 2) {
        fprintf(stderr, "error: missing KERNEL\n");
        return false;
    }
    *args = (Args){.kernel = argv[1], .repeats = 1, .threads = 1, .data = "."};
    for (int i = 2; i < argc; i += 2) {
        const char *flag = argv[i];
        if (i + 1 >= argc) {
            fprintf(stderr, "error: %s needs a value\n", flag);
            return false;
        }
        const char *value = argv[i + 1];
        bool ok = true;
        if (strcmp(flag, "--size") == 0) {
            ok = parse_count(flag, value, &args->size);
        } else if (strcmp(flag, "--repeats") == 0) {
            ok = parse_count(flag, value, &args->repeats);
        } else if (strcmp(flag, "--threads") == 0) {
            ok = parse_count(flag, value, &args->threads);
        } else if (strcmp(flag, "--data") == 0) {
            args->data = value;
        } else {
            fprintf(stderr, "error: unknown option %s\n", flag);
            return false;
        }
        if (!ok) {
            return false;
        }
    }
    if (args->size == 0) {
        fprintf(stderr, "error: --size is required\n");
        return false;
    }
    return true;
}

static void print_json_string(const char *s) {
    putchar('"');
    for (; *s != '\0'; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') {
            printf("\\%c", c);
        } else if (c < 0x20) {
            printf("\\u%04x", c);
        } else {
            putchar(c);
        }
    }
    putchar('"');
}

static void print_row(const Args *args, const Result *r) {
    printf("{\"kernel\":");
    print_json_string(args->kernel);
    printf(",\"size\":%zu,\"threads\":%zu,\"checksum\":", args->size, args->threads);
    print_json_string(r->checksum);
    printf(",\"times_ms\":[");
    for (size_t i = 0; i < args->repeats; i++) {
        printf("%s%.3f", i ? "," : "", r->times[i]);
    }
    printf("],\"runtime\":");
    char runtime[1024];
    const char *libs = kernel_libraries(args->kernel);
    snprintf(runtime, sizeof runtime, "C17, %s, %s%s%s", COMPILER, KERNELS_BUILD, libs ? "; " : "",
             libs ? libs : "");
    print_json_string(runtime);
    if (r->extra[0] != '\0') {
        printf(",\"extra\":%s", r->extra);
    }
    printf("}\n");
}

int main(int argc, char **argv) {
    Args args;
    if (!parse_args(argc, argv, &args)) {
        fprintf(stderr,
                "usage: kernels KERNEL --size N [--repeats R] [--threads T] [--data DIR]\n");
        return 2;
    }
    const Kernel *kernel = find_kernel(args.kernel);
    if (kernel == NULL) {
        fprintf(stderr, "unknown kernel %s\n", args.kernel);
        return 2;
    }
    Result result = {.times = xcalloc(args.repeats, sizeof(double))};
    if (!kernel->run(&args, &result)) {
        return EXIT_FAILURE;
    }
    print_row(&args, &result);
    free(result.times);
    return EXIT_SUCCESS;
}
