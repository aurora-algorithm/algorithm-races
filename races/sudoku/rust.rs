// Race: sudoku (rust)
// Extracted from suite/rust/src/puzzle.rs — build and run via suite/run.py

use std::cmp::Reverse;
use std::collections::BinaryHeap;
use std::iter;
use num_bigint::BigInt;
use crate::inputs::{XorShift32, fnv1a, h31, hashmap_input, ints};
use crate::puzzle_text::{
    aho_corasick, aho_input, binary_text, bwt, dna_text, huffman, palindrome, skewed_text,
};
use crate::work::Outcome;
use crate::{Args, measure};

fn shuffle<T>(rng: &mut XorShift32, items: &mut [T]) {
    for i in (1..items.len()).rev() {
        let j = rng.next_u32() as usize % (i + 1);
        items.swap(i, j);
    }
}

const DLX_COLUMNS: usize = 324;

fn sudoku_puzzles(n: usize) -> Vec<[u8; 81]> {
    let mut rng = XorShift32::new();
    (0..n)
        .map(|_| {
            let mut digits: Vec<u8> = (1..=9).collect();
            shuffle(&mut rng, &mut digits);
            let rows = shuffled_lines(&mut rng);
            let cols = shuffled_lines(&mut rng);
            let mut cells: Vec<usize> = (0..81).collect();
            shuffle(&mut rng, &mut cells);
            let mut puzzle = [0u8; 81];
            for &cell in &cells[..SUDOKU_CLUES] {
                let (r, c) = (rows[cell / 9], cols[cell % 9]);
                puzzle[cell] = digits[(r * 3 + r / 3 + c) % 9];
            }
            puzzle
        })
        .collect()
}

/// Rows (or columns) of a valid grid, shuffled within and between their bands.
fn shuffled_lines(rng: &mut XorShift32) -> Vec<usize> {
    let mut bands = [0, 1, 2];
    shuffle(rng, &mut bands);
    let mut lines = Vec::with_capacity(9);
    for band in bands {
        let mut inner = [0, 1, 2];
        shuffle(rng, &mut inner);
        lines.extend(inner.iter().map(|i| band * 3 + i));
    }
    lines
}

/// The exact-cover matrix as flat arrays: node 0 is the root, 1..324 the column headers, then 4 nodes per row.
struct Dlx {
    left: Vec<usize>,
    right: Vec<usize>,
    up: Vec<usize>,
    down: Vec<usize>,
    col: Vec<usize>,
    row: Vec<usize>,
    count: Vec<u32>,
    solutions: u64,
    tries: u64,
    chosen: [usize; 81],
    first: Vec<usize>,
}

impl Dlx {
    fn new() -> Self {
        let size = 1 + DLX_COLUMNS + 729 * 4;
        let mut dlx = Dlx {
            left: vec![0; size],
            right: vec![0; size],
            up: (0..size).collect(),
            down: (0..size).collect(),
            col: (0..size).collect(),
            row: vec![0; size],
            count: vec![0; DLX_COLUMNS + 1],
            solutions: 0,
            tries: 0,
            chosen: [0; 81],
            first: Vec::new(),
        };
        for i in 0..=DLX_COLUMNS {
            dlx.left[i] = (i + DLX_COLUMNS) % (DLX_COLUMNS + 1);
            dlx.right[i] = (i + 1) % (DLX_COLUMNS + 1);
        }
        let mut node = DLX_COLUMNS + 1;
        for r in 0..9 {
            for c in 0..9 {
                let b = (r / 3) * 3 + c / 3;
                for d in 0..9 {
                    let columns = [
                        1 + r * 9 + c,
                        82 + r * 9 + d,
                        163 + c * 9 + d,
                        244 + b * 9 + d,
                    ];
                    dlx.add_row(node, (r * 9 + c) * 9 + d, columns);
                    node += 4;
                }
            }
        }
        dlx
    }

    /// Appends the four nodes `first..first + 4` at the bottom of their columns and links them in a ring.
    fn add_row(&mut self, first: usize, row: usize, columns: [usize; 4]) {
        for (k, column) in columns.into_iter().enumerate() {
            let node = first + k;
            self.col[node] = column;
            self.row[node] = row;
            self.up[node] = self.up[column];
            self.down[node] = column;
            self.down[self.up[column]] = node;
            self.up[column] = node;
            self.count[column] += 1;
            self.left[node] = node - 1;
            self.right[node] = node + 1;
        }
        self.left[first] = first + 3;
        self.right[first + 3] = first;
    }

    fn cover(&mut self, c: usize) {
        self.right[self.left[c]] = self.right[c];
        self.left[self.right[c]] = self.left[c];
        let mut i = self.down[c];
        while i != c {
            let mut j = self.right[i];
            while j != i {
                self.down[self.up[j]] = self.down[j];
                self.up[self.down[j]] = self.up[j];
                self.count[self.col[j]] -= 1;
                j = self.right[j];
            }
            i = self.down[i];
        }
    }

    fn uncover(&mut self, c: usize) {
        let mut i = self.up[c];
        while i != c {
            let mut j = self.left[i];
            while j != i {
                self.count[self.col[j]] += 1;
                self.down[self.up[j]] = j;
                self.up[self.down[j]] = j;
                j = self.left[j];
            }
            i = self.up[i];
        }
        self.right[self.left[c]] = c;
        self.left[self.right[c]] = c;
    }

    fn choose_row(&mut self, node: usize) {
        let mut j = self.right[node];
        while j != node {
            self.cover(self.col[j]);
            j = self.right[j];
        }
    }

    fn unchoose_row(&mut self, node: usize) {
        let mut j = self.left[node];
        while j != node {
            self.uncover(self.col[j]);
            j = self.left[j];
        }
    }

    fn search(&mut self, depth: usize) {
        if self.right[0] == 0 {
            self.solutions += 1;
            if self.solutions == 1 {
                self.first = self.chosen[..depth].to_vec();
            }
            return;
        }
        let mut best = self.right[0];
        let mut c = self.right[best];
        while c != 0 {
            if self.count[c] < self.count[best] {
                best = c;
            }
            c = self.right[c];
        }
        self.cover(best);
        let mut r = self.down[best];
        while r != best {
            self.tries += 1;
            self.chosen[depth] = self.row[r];
            self.choose_row(r);
            self.search(depth + 1);
            self.unchoose_row(r);
            r = self.down[r];
        }
        self.uncover(best);
    }
}

fn sudoku(puzzles: &[[u8; 81]]) -> String {
    let (mut solutions, mut tries, mut h) = (0, 0, 0u32);
    for puzzle in puzzles {
        let mut dlx = Dlx::new();
        let mut depth = 0;
        for (cell, &digit) in puzzle.iter().enumerate() {
            if digit > 0 {
                let row = cell * 9 + usize::from(digit) - 1;
                let node = DLX_COLUMNS + 1 + row * 4;
                dlx.cover(dlx.col[node]);
                dlx.choose_row(node);
                dlx.chosen[depth] = row;
                depth += 1;
            }
        }
        dlx.search(depth);
        solutions += dlx.solutions;
        tries += dlx.tries;
        let mut grid = [0i64; 81];
        for &row in &dlx.first {
            grid[row / 9] = (row % 9 + 1) as i64;
        }
        h = h31(iter::once(i64::from(h)).chain(grid));
    }
    format!("{}:{solutions}:{tries}:{h}", puzzles.len())
}
