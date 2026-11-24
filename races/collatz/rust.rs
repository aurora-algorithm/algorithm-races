// Race: collatz (rust)
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

fn collatz(n: usize) -> String {
    let mut cache = vec![0u32; n];
    cache[1] = 1;
    let (mut best, mut best_len, mut total) = (1, 1, 1u64);
    for start in 2..n {
        let mut x = start as u64;
        let mut steps = 0;
        while x >= start as u64 {
            x = if x.is_multiple_of(2) {
                x / 2
            } else {
                3 * x + 1
            };
            steps += 1;
        }
        let len = steps + cache[x as usize];
        cache[start] = len;
        total += u64::from(len);
        if len > best_len {
            best = start;
            best_len = len;
        }
    }
    format!("{n}:{best}:{best_len}:{total}")
}
