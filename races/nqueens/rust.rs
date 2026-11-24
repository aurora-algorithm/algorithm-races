// Race: nqueens (rust)
// Extracted from suite/rust/src/algo.rs — build and run via suite/run.py

use std::cmp::{Ordering, Reverse};
use std::collections::{BTreeMap, BinaryHeap, HashMap, VecDeque};
use crate::inputs::{XorShift32, h31, hashmap_input, ints};
use crate::work::Outcome;
use crate::{Args, measure};

struct Board {
    n: usize,
    cols: Vec<bool>,
    up: Vec<bool>,
    down: Vec<bool>,
}

impl Board {
    fn place(&mut self, row: usize) -> u64 {
        if row == self.n {
            return 1;
        }
        let mut count = 0;
        for col in 0..self.n {
            let (u, d) = (row + col, row + self.n - 1 - col);
            if !self.cols[col] && !self.up[u] && !self.down[d] {
                (self.cols[col], self.up[u], self.down[d]) = (true, true, true);
                count += self.place(row + 1);
                (self.cols[col], self.up[u], self.down[d]) = (false, false, false);
            }
        }
        count
    }
}

fn nqueens(n: usize) -> u64 {
    let mut board = Board {
        n,
        cols: vec![false; n],
        up: vec![false; 2 * n - 1],
        down: vec![false; 2 * n - 1],
    };
    board.place(0)
}
