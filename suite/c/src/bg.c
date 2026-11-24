/* Benchmarks Game kernels, single-threaded (SPEC.md section 4). Floating-point code keeps the
 * operation order of the spec so every language prints the same digits. */
#include "bg.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernels.h"

typedef struct Node {
    struct Node *left;
    struct Node *right;
} Node;

static Node *make_tree(unsigned depth) {
    Node *node = xmalloc(sizeof *node);
    if (depth == 0) {
        node->left = node->right = NULL;
    } else {
        node->left = make_tree(depth - 1);
        node->right = make_tree(depth - 1);
    }
    return node;
}

static uint64_t check_tree(const Node *node) {
    if (node->left == NULL) {
        return 1;
    }
    return 1 + check_tree(node->left) + check_tree(node->right);
}

static void free_tree(Node *node) {
    if (node->left != NULL) {
        free_tree(node->left);
        free_tree(node->right);
    }
    free(node);
}

uint64_t depth_checks(unsigned depth, uint64_t iterations) {
    uint64_t sum = 0;
    for (uint64_t i = 0; i < iterations; i++) {
        Node *tree = make_tree(depth);
        sum += check_tree(tree);
        free_tree(tree);
    }
    return sum;
}

void binary_trees_text(unsigned n, DepthRun run, const void *ctx, Bytes *out) {
    unsigned max_depth = n > MIN_DEPTH + 2 ? n : MIN_DEPTH + 2;
    unsigned stretch = max_depth + 1;
    out->len = 0;
    bytes_printf(out, "stretch tree of depth %u\t check: %" PRIu64 "\n", stretch,
                 depth_checks(stretch, 1));
    Node *long_lived = make_tree(max_depth);
    for (unsigned depth = MIN_DEPTH; depth <= max_depth; depth += 2) {
        uint64_t iterations = (uint64_t)1 << (max_depth - depth + MIN_DEPTH);
        uint64_t check = run(depth, iterations, ctx);
        bytes_printf(out, "%" PRIu64 "\t trees of depth %u\t check: %" PRIu64 "\n", iterations,
                     depth, check);
    }
    bytes_printf(out, "long lived tree of depth %u\t check: %" PRIu64 "\n", max_depth,
                 check_tree(long_lived));
    free_tree(long_lived);
}

bool run_binary_trees(const Args *args, Result *out, DepthRun run) {
    if (args->size > 40) {
        fprintf(stderr, "error: --size %zu is too deep for a tree\n", args->size);
        return false;
    }
    Bytes text = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        binary_trees_text((unsigned)args->size, run, &args->threads, &text);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu32, fnv1a(text.data, text.len));
    free(text.data);
    return true;
}

static uint64_t sequential_depth(unsigned depth, uint64_t iterations, const void *ctx) {
    (void)ctx;
    return depth_checks(depth, iterations);
}

static bool binary_trees(const Args *args, Result *out) {
    return run_binary_trees(args, out, sequential_depth);
}

#define PI 3.141592653589793
#define SOLAR_MASS (4.0 * PI * PI)
#define DAYS_PER_YEAR 365.24
#define BODY_COUNT 5

typedef struct {
    double x, y, z, vx, vy, vz, mass;
} Body;

/* The Benchmarks Game's literals, kept digit for digit. */
static const Body BODIES[BODY_COUNT] = {
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, SOLAR_MASS}, /* Sun */
    {4.84143144246472090e00, -1.16032004402742839e00, -1.03622044471123109e-01,
     1.66007664274403694e-03 * DAYS_PER_YEAR, 7.69901118419740425e-03 * DAYS_PER_YEAR,
     -6.90460016972063023e-05 * DAYS_PER_YEAR, 9.54791938424326609e-04 * SOLAR_MASS}, /* Jupiter */
    {8.34336671824457987e00, 4.12479856412430479e00, -4.03523417114321381e-01,
     -2.76742510726862411e-03 * DAYS_PER_YEAR, 4.99852801234917238e-03 * DAYS_PER_YEAR,
     2.30417297573763929e-05 * DAYS_PER_YEAR, 2.85885980666130812e-04 * SOLAR_MASS}, /* Saturn */
    {1.28943695621391310e01, -1.51111514016986312e01, -2.23307578892655734e-01,
     2.96460137564761618e-03 * DAYS_PER_YEAR, 2.37847173959480950e-03 * DAYS_PER_YEAR,
     -2.96589568540237556e-05 * DAYS_PER_YEAR, 4.36624404335156298e-05 * SOLAR_MASS}, /* Uranus */
    {1.53796971148509165e01, -2.59193146099879641e01, 1.79258772950371181e-01,
     2.68067772490389322e-03 * DAYS_PER_YEAR, 1.62824170038242295e-03 * DAYS_PER_YEAR,
     -9.51592254519715870e-05 * DAYS_PER_YEAR, 5.15138902046611451e-05 * SOLAR_MASS}, /* Neptune */
};

static void offset_momentum(Body *bodies) {
    double px = 0.0, py = 0.0, pz = 0.0;
    for (int i = 0; i < BODY_COUNT; i++) {
        px += bodies[i].vx * bodies[i].mass;
        py += bodies[i].vy * bodies[i].mass;
        pz += bodies[i].vz * bodies[i].mass;
    }
    bodies[0].vx = -px / SOLAR_MASS;
    bodies[0].vy = -py / SOLAR_MASS;
    bodies[0].vz = -pz / SOLAR_MASS;
}

static double energy(const Body *bodies) {
    double e = 0.0;
    for (int i = 0; i < BODY_COUNT; i++) {
        const Body *bi = &bodies[i];
        e += 0.5 * bi->mass * (bi->vx * bi->vx + bi->vy * bi->vy + bi->vz * bi->vz);
        for (int j = i + 1; j < BODY_COUNT; j++) {
            const Body *bj = &bodies[j];
            double dx = bi->x - bj->x;
            double dy = bi->y - bj->y;
            double dz = bi->z - bj->z;
            e -= (bi->mass * bj->mass) / sqrt(dx * dx + dy * dy + dz * dz);
        }
    }
    return e;
}

static void advance(Body *bodies, double dt, size_t steps) {
    for (size_t s = 0; s < steps; s++) {
        for (int i = 0; i < BODY_COUNT; i++) {
            Body *bi = &bodies[i];
            for (int j = i + 1; j < BODY_COUNT; j++) {
                Body *bj = &bodies[j];
                double dx = bi->x - bj->x;
                double dy = bi->y - bj->y;
                double dz = bi->z - bj->z;
                double d2 = dx * dx + dy * dy + dz * dz;
                double mag = dt / (d2 * sqrt(d2));
                double bim = bi->mass * mag;
                double bjm = bj->mass * mag;
                bi->vx -= dx * bjm;
                bi->vy -= dy * bjm;
                bi->vz -= dz * bjm;
                bj->vx += dx * bim;
                bj->vy += dy * bim;
                bj->vz += dz * bim;
            }
        }
        for (int i = 0; i < BODY_COUNT; i++) {
            bodies[i].x += dt * bodies[i].vx;
            bodies[i].y += dt * bodies[i].vy;
            bodies[i].z += dt * bodies[i].vz;
        }
    }
}

static bool nbody(const Args *args, Result *out) {
    double before = 0.0, after = 0.0;
    for (size_t r = 0; r < args->repeats; r++) {
        Body bodies[BODY_COUNT];
        double start = now_ms();
        memcpy(bodies, BODIES, sizeof bodies);
        offset_momentum(bodies);
        before = energy(bodies);
        advance(bodies, 0.01, args->size);
        after = energy(bodies);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%.9f;%.9f", before, after);
    return true;
}

size_t row_bytes(size_t n) {
    return (n + 7) / 8;
}

void mandelbrot_row(size_t n, size_t y, unsigned char *row) {
    double ci = 2.0 * (double)y / (double)n - 1.0;
    for (size_t x = 0; x < n; x++) {
        double cr = 2.0 * (double)x / (double)n - 1.5;
        double zr = 0.0, zi = 0.0, tr = 0.0, ti = 0.0;
        for (int i = 0; i < 50 && tr + ti <= 4.0; i++) {
            zi = 2.0 * zr * zi + ci;
            zr = tr - ti + cr;
            tr = zr * zr;
            ti = zi * zi;
        }
        if (tr + ti <= 4.0) {
            row[x >> 3] |= (unsigned char)(0x80u >> (x & 7));
        }
    }
}

static bool mandelbrot(const Args *args, Result *out) {
    size_t n = args->size;
    size_t bytes = n * row_bytes(n);
    unsigned char *bitmap = NULL;
    for (size_t r = 0; r < args->repeats; r++) {
        free(bitmap);
        double start = now_ms();
        bitmap = xcalloc(bytes, 1);
        for (size_t y = 0; y < n; y++) {
            mandelbrot_row(n, y, bitmap + y * row_bytes(n));
        }
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%" PRIu32, fnv1a(bitmap, bytes));
    free(bitmap);
    return true;
}

/* The Benchmarks Game's sequential algorithm; returns "{checksum}:{maxflips}". */
static void fannkuch_run(size_t n, char *checksum_out) {
    size_t *perm1 = xmalloc(n * sizeof *perm1);
    size_t *perm = xmalloc(n * sizeof *perm);
    size_t *count = xcalloc(n, sizeof *count);
    for (size_t i = 0; i < n; i++) {
        perm1[i] = i;
    }
    int64_t max_flips = 0, checksum = 0;
    uint64_t perm_count = 0;
    size_t r = n;
    for (;;) {
        for (; r != 1; r--) {
            count[r - 1] = r;
        }
        memcpy(perm, perm1, n * sizeof *perm);
        int64_t flips = 0;
        for (size_t k = perm[0]; k != 0; k = perm[0]) {
            for (size_t i = 0, j = k; i < j; i++, j--) {
                size_t t = perm[i];
                perm[i] = perm[j];
                perm[j] = t;
            }
            flips++;
        }
        if (flips > max_flips) {
            max_flips = flips;
        }
        checksum += perm_count % 2 == 0 ? flips : -flips;
        for (;;) {
            if (r == n) {
                snprintf(checksum_out, CHECKSUM_MAX, "%" PRId64 ":%" PRId64, checksum, max_flips);
                free(perm1);
                free(perm);
                free(count);
                return;
            }
            size_t first = perm1[0];
            memmove(perm1, perm1 + 1, r * sizeof *perm1);
            perm1[r] = first;
            if (--count[r] > 0) {
                break;
            }
            r++;
        }
        perm_count++;
    }
}

static bool fannkuch(const Args *args, Result *out) {
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        fannkuch_run(args->size, out->checksum);
        out->times[r] = now_ms() - start;
    }
    return true;
}

static double eval_a(size_t i, size_t j) {
    return 1.0 / (double)((i + j) * (i + j + 1) / 2 + i + 1);
}

static void mul_av(const double *u, double *v, size_t n) {
    for (size_t i = 0; i < n; i++) {
        double s = 0.0;
        for (size_t j = 0; j < n; j++) {
            s += eval_a(i, j) * u[j];
        }
        v[i] = s;
    }
}

static void mul_atv(const double *u, double *v, size_t n) {
    for (size_t i = 0; i < n; i++) {
        double s = 0.0;
        for (size_t j = 0; j < n; j++) {
            s += eval_a(j, i) * u[j];
        }
        v[i] = s;
    }
}

static void mul_atav(const double *u, double *v, double *tmp, size_t n) {
    mul_av(u, tmp, n);
    mul_atv(tmp, v, n);
}

static double spectral_norm_run(size_t n) {
    double *u = xmalloc(n * sizeof *u);
    double *v = xmalloc(n * sizeof *v);
    double *tmp = xmalloc(n * sizeof *tmp);
    for (size_t i = 0; i < n; i++) {
        u[i] = 1.0;
    }
    for (int i = 0; i < 10; i++) {
        mul_atav(u, v, tmp, n);
        mul_atav(v, u, tmp, n);
    }
    double vbv = 0.0, vv = 0.0;
    for (size_t i = 0; i < n; i++) {
        vbv += u[i] * v[i];
        vv += v[i] * v[i];
    }
    free(u);
    free(v);
    free(tmp);
    return sqrt(vbv / vv);
}

static bool spectral_norm(const Args *args, Result *out) {
    double result = 0.0;
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        result = spectral_norm_run(args->size);
        out->times[r] = now_ms() - start;
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%.9f", result);
    return true;
}

const Kernel BG_KERNELS[] = {
    {"binary-trees", binary_trees},   {"nbody", nbody},
    {"mandelbrot", mandelbrot},       {"fannkuch", fannkuch},
    {"spectral-norm", spectral_norm}, {NULL, NULL},
};
