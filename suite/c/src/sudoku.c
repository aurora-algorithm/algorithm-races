/* sudoku (SPEC.md section 13): Knuth's Algorithm X with dancing links, in flat arrays. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernels.h"
#include "puzzle.h"

#define SUDOKU_CLUES 26
#define DLX_COLUMNS 324
#define DLX_SIZE (1 + DLX_COLUMNS + 729 * 4)

/* Rows (or columns) of a valid grid, shuffled within and between their bands. */
static void shuffled_lines(Rng *rng, size_t lines[9]) {
    size_t bands[3] = {0, 1, 2};
    shuffle(rng, bands, 3);
    for (size_t b = 0; b < 3; b++) {
        size_t inner[3] = {0, 1, 2};
        shuffle(rng, inner, 3);
        for (size_t i = 0; i < 3; i++) {
            lines[b * 3 + i] = bands[b] * 3 + inner[i];
        }
    }
}

static unsigned char *sudoku_puzzles(size_t n) {
    Rng rng = rng_new();
    unsigned char *puzzles = xcalloc(n, 81);
    for (size_t p = 0; p < n; p++) {
        size_t digits[9], rows[9], cols[9], cells[81];
        for (size_t i = 0; i < 9; i++) {
            digits[i] = i + 1;
        }
        shuffle(&rng, digits, 9);
        shuffled_lines(&rng, rows);
        shuffled_lines(&rng, cols);
        for (size_t i = 0; i < 81; i++) {
            cells[i] = i;
        }
        shuffle(&rng, cells, 81);
        for (size_t i = 0; i < SUDOKU_CLUES; i++) {
            size_t cell = cells[i];
            size_t r = rows[cell / 9], c = cols[cell % 9];
            puzzles[p * 81 + cell] = (unsigned char)digits[(r * 3 + r / 3 + c) % 9];
        }
    }
    return puzzles;
}

/* The exact-cover matrix: node 0 is the root, 1..324 the column headers, then 4 nodes per row. */
typedef struct {
    size_t left[DLX_SIZE], right[DLX_SIZE], up[DLX_SIZE], down[DLX_SIZE], col[DLX_SIZE],
        row[DLX_SIZE];
    uint32_t count[DLX_COLUMNS + 1];
    uint64_t solutions, tries;
    size_t chosen[81];
    size_t first[81];
    size_t first_len;
} Dlx;

/* Appends the four nodes first..first+3 at the bottom of their columns and links them in a ring. */
static void add_row(Dlx *x, size_t first, size_t row, const size_t columns[4]) {
    for (size_t k = 0; k < 4; k++) {
        size_t node = first + k, column = columns[k];
        x->col[node] = column;
        x->row[node] = row;
        x->up[node] = x->up[column];
        x->down[node] = column;
        x->down[x->up[column]] = node;
        x->up[column] = node;
        x->count[column]++;
        x->left[node] = node - 1;
        x->right[node] = node + 1;
    }
    x->left[first] = first + 3;
    x->right[first + 3] = first;
}

static Dlx *dlx_new(void) {
    Dlx *x = xcalloc(1, sizeof *x);
    for (size_t i = 0; i < DLX_SIZE; i++) {
        x->up[i] = x->down[i] = x->col[i] = i;
    }
    for (size_t i = 0; i <= DLX_COLUMNS; i++) {
        x->left[i] = (i + DLX_COLUMNS) % (DLX_COLUMNS + 1);
        x->right[i] = (i + 1) % (DLX_COLUMNS + 1);
    }
    size_t node = DLX_COLUMNS + 1;
    for (size_t r = 0; r < 9; r++) {
        for (size_t c = 0; c < 9; c++) {
            size_t b = (r / 3) * 3 + c / 3;
            for (size_t d = 0; d < 9; d++) {
                const size_t columns[4] = {1 + r * 9 + c, 82 + r * 9 + d, 163 + c * 9 + d,
                                           244 + b * 9 + d};
                add_row(x, node, (r * 9 + c) * 9 + d, columns);
                node += 4;
            }
        }
    }
    return x;
}

static void cover(Dlx *x, size_t c) {
    x->right[x->left[c]] = x->right[c];
    x->left[x->right[c]] = x->left[c];
    for (size_t i = x->down[c]; i != c; i = x->down[i]) {
        for (size_t j = x->right[i]; j != i; j = x->right[j]) {
            x->down[x->up[j]] = x->down[j];
            x->up[x->down[j]] = x->up[j];
            x->count[x->col[j]]--;
        }
    }
}

static void uncover(Dlx *x, size_t c) {
    for (size_t i = x->up[c]; i != c; i = x->up[i]) {
        for (size_t j = x->left[i]; j != i; j = x->left[j]) {
            x->count[x->col[j]]++;
            x->down[x->up[j]] = j;
            x->up[x->down[j]] = j;
        }
    }
    x->right[x->left[c]] = c;
    x->left[x->right[c]] = c;
}

static void choose_row(Dlx *x, size_t node) {
    for (size_t j = x->right[node]; j != node; j = x->right[j]) {
        cover(x, x->col[j]);
    }
}

static void unchoose_row(Dlx *x, size_t node) {
    for (size_t j = x->left[node]; j != node; j = x->left[j]) {
        uncover(x, x->col[j]);
    }
}

static void search(Dlx *x, size_t depth) {
    if (x->right[0] == 0) {
        x->solutions++;
        if (x->solutions == 1) {
            memcpy(x->first, x->chosen, depth * sizeof *x->chosen);
            x->first_len = depth;
        }
        return;
    }
    size_t best = x->right[0];
    for (size_t c = x->right[best]; c != 0; c = x->right[c]) {
        if (x->count[c] < x->count[best]) {
            best = c;
        }
    }
    cover(x, best);
    for (size_t r = x->down[best]; r != best; r = x->down[r]) {
        x->tries++;
        x->chosen[depth] = x->row[r];
        choose_row(x, r);
        search(x, depth + 1);
        unchoose_row(x, r);
    }
    uncover(x, best);
}

typedef struct {
    uint64_t solutions, tries;
} SudokuResult;

/* Solves every puzzle; the first solution of each goes into grids (81 digits per puzzle). */
static SudokuResult sudoku_run(const unsigned char *puzzles, size_t n, int64_t *grids) {
    uint64_t solutions = 0, tries = 0;
    for (size_t p = 0; p < n; p++) {
        const unsigned char *puzzle = puzzles + p * 81;
        Dlx *x = dlx_new();
        size_t depth = 0;
        for (size_t cell = 0; cell < 81; cell++) {
            if (puzzle[cell] > 0) {
                size_t row = cell * 9 + puzzle[cell] - 1;
                size_t node = DLX_COLUMNS + 1 + row * 4;
                cover(x, x->col[node]);
                choose_row(x, node);
                x->chosen[depth++] = row;
            }
        }
        search(x, depth);
        solutions += x->solutions;
        tries += x->tries;
        int64_t *grid = grids + p * 81;
        memset(grid, 0, 81 * sizeof *grid);
        for (size_t i = 0; i < x->first_len; i++) {
            grid[x->first[i] / 9] = (int64_t)(x->first[i] % 9 + 1);
        }
        free(x);
    }
    return (SudokuResult){solutions, tries};
}

static bool sudoku(const Args *args, Result *out) {
    size_t n = args->size;
    unsigned char *puzzles = sudoku_puzzles(n);
    int64_t *grids = xmalloc(n * 81 * sizeof *grids);
    SudokuResult res = {0};
    for (size_t r = 0; r < args->repeats; r++) {
        double start = now_ms();
        res = sudoku_run(puzzles, n, grids);
        out->times[r] = now_ms() - start;
    }
    uint32_t h = 0;
    for (size_t p = 0; p < n; p++) {
        h = h31_step(0, h);
        for (size_t i = 0; i < 81; i++) {
            h = h31_step(h, grids[p * 81 + i]);
        }
    }
    snprintf(out->checksum, CHECKSUM_MAX, "%zu:%" PRIu64 ":%" PRIu64 ":%" PRIu32, n, res.solutions,
             res.tries, h);
    free(grids);
    free(puzzles);
    return true;
}

const Kernel SUDOKU_KERNELS[] = {{"sudoku", sudoku}, {NULL, NULL}};
