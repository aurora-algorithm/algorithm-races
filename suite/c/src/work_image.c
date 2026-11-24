/* Work kernel image (SPEC.md section 11): stb_image decodes, stb_image_resize2 resizes with its
 * default filter, stb_image_write encodes JPEG quality 80; all three in memory. */
#include <stdio.h>
#include <stdlib.h>

#include <stb_image.h>
#include <stb_image_resize2.h>
#include <stb_image_write.h>

#include "kernels.h"

#define PHOTOS 8
#define TARGET_WIDTH 1080
#define JPEG_QUALITY 80

static void append_jpeg(void *ctx, void *data, int size) {
    bytes_append(ctx, data, (size_t)size);
}

typedef struct {
    int width, height;
    size_t out_bytes;
} Converted;

static bool convert_all(unsigned char *const *photos, const size_t *lens, size_t n,
                        Converted *res) {
    *res = (Converted){0};
    Bytes jpeg = {0};
    bool ok = true;
    for (size_t i = 0; i < n && ok; i++) {
        int w, h, channels;
        unsigned char *pixels =
            stbi_load_from_memory(photos[i], (int)lens[i], &w, &h, &channels, 3);
        if (pixels == NULL) {
            fprintf(stderr, "error: stb_image: %s\n", stbi_failure_reason());
            ok = false;
            break;
        }
        int th = (h * TARGET_WIDTH + w / 2) / w;
        unsigned char *small =
            stbir_resize_uint8_linear(pixels, w, h, 0, NULL, TARGET_WIDTH, th, 0, STBIR_RGB);
        stbi_image_free(pixels);
        jpeg.len = 0;
        ok = small != NULL && stbi_write_jpg_to_func(append_jpeg, &jpeg, TARGET_WIDTH, th, 3, small,
                                                     JPEG_QUALITY) != 0;
        free(small);
        if (!ok) {
            fprintf(stderr, "error: resizing or encoding photo %zu failed\n", i);
            break;
        }
        *res = (Converted){TARGET_WIDTH, th, res->out_bytes + jpeg.len};
    }
    free(jpeg.data);
    return ok;
}

static bool image(const Args *args, Result *out) {
    size_t n = args->size;
    unsigned char **photos = xcalloc(n, sizeof *photos);
    size_t *lens = xcalloc(n, sizeof *lens);
    bool ok = true;
    for (size_t i = 0; i < n && ok; i++) {
        char name[64];
        snprintf(name, sizeof name, "photos/photo-%zu.jpg", i % PHOTOS);
        char *path = path_join(args->data, name);
        photos[i] = read_file(path, &lens[i]);
        ok = photos[i] != NULL;
        free(path);
    }
    Converted res = {0};
    for (size_t r = 0; r < args->repeats && ok; r++) {
        double start = now_ms();
        ok = convert_all(photos, lens, n, &res);
        out->times[r] = now_ms() - start;
    }
    if (ok) {
        snprintf(out->checksum, CHECKSUM_MAX, "%zu:%dx%d", n, res.width, res.height);
        snprintf(out->extra, EXTRA_MAX, "{\"out_bytes\":%zu}", res.out_bytes);
    }
    for (size_t i = 0; i < n; i++) {
        free(photos[i]);
    }
    free(photos);
    free(lens);
    return ok;
}

const Kernel IMAGE_KERNELS[] = {{"image", image}, {NULL, NULL}};
