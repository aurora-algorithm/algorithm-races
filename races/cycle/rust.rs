// Race: cycle (rust)
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

const CYCLE_MOD: u64 = 67108859;

const CYCLE_C: u64 = 12345;

fn cycle_starts(n: usize) -> Vec<u64> {
    let mut rng = XorShift32::new();
    (0..n)
        .map(|_| u64::from(rng.next_u32()) % CYCLE_MOD)
        .collect()
}

fn step(x: u64) -> u64 {
    (x * x + CYCLE_C) % CYCLE_MOD
}

fn cycle(starts: &[u64]) -> String {
    let (mut total_mu, mut total_lam, mut longest) = (0u64, 0u64, 0u64);
    for &x0 in starts {
        let mut tortoise = step(x0);
        let mut hare = step(step(x0));
        while tortoise != hare {
            tortoise = step(tortoise);
            hare = step(step(hare));
        }
        let mut mu = 0;
        tortoise = x0;
        while tortoise != hare {
            tortoise = step(tortoise);
            hare = step(hare);
            mu += 1;
        }
        let mut lam = 1;
        hare = step(tortoise);
        while tortoise != hare {
            hare = step(hare);
            lam += 1;
        }
        total_mu += mu;
        total_lam += lam;
        longest = longest.max(mu + lam);
    }
    format!("{}:{total_mu}:{total_lam}:{longest}", starts.len())
}
