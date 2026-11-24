// Race: life (rust)
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

const LIFE_SIDE: usize = 512;

fn life_input() -> Vec<u8> {
    let mut rng = XorShift32::new();
    (0..LIFE_SIDE * LIFE_SIDE)
        .map(|_| u8::from(rng.next_u32().is_multiple_of(4)))
        .collect()
}

fn life(grid: &[u8], generations: usize) -> Vec<u8> {
    let w = LIFE_SIDE;
    let mut cur = grid.to_vec();
    let mut next = vec![0u8; grid.len()];
    for _ in 0..generations {
        for y in 0..w {
            let up = (y + w - 1) % w * w;
            let row = y * w;
            let down = (y + 1) % w * w;
            for x in 0..w {
                let left = (x + w - 1) % w;
                let right = (x + 1) % w;
                let n = cur[up + left]
                    + cur[up + x]
                    + cur[up + right]
                    + cur[row + left]
                    + cur[row + right]
                    + cur[down + left]
                    + cur[down + x]
                    + cur[down + right];
                next[row + x] = u8::from(n == 3 || (n == 2 && cur[row + x] == 1));
            }
        }
        std::mem::swap(&mut cur, &mut next);
    }
    cur
}
