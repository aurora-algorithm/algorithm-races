// Race: rain-water (rust)
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

fn terrain(side: usize) -> Vec<u64> {
    let mut rng = XorShift32::new();
    (0..side * side)
        .map(|_| u64::from(rng.next_u32() % 1000))
        .collect()
}

fn rain_water(side: usize, heights: &[u64]) -> String {
    let n = heights.len();
    let mut seen = vec![false; n];
    let mut heap = BinaryHeap::new();
    for idx in 0..n {
        let (r, c) = (idx / side, idx % side);
        if r == 0 || r == side - 1 || c == 0 || c == side - 1 {
            seen[idx] = true;
            heap.push(Reverse(heights[idx] * n as u64 + idx as u64));
        }
    }
    let (mut water, mut wet) = (0u64, 0u64);
    while let Some(Reverse(key)) = heap.pop() {
        let level = key / n as u64;
        let idx = (key % n as u64) as usize;
        let (r, c) = (idx / side, idx % side);
        let neighbours = [
            (r > 0).then(|| idx - side),
            (r + 1 < side).then(|| idx + side),
            (c > 0).then(|| idx - 1),
            (c + 1 < side).then(|| idx + 1),
        ];
        for next in neighbours.into_iter().flatten() {
            if seen[next] {
                continue;
            }
            seen[next] = true;
            let mut h = heights[next];
            if h < level {
                water += level - h;
                wet += 1;
                h = level;
            }
            heap.push(Reverse(h * n as u64 + next as u64));
        }
    }
    format!("{side}:{water}:{wet}")
}
