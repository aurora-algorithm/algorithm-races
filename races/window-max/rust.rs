// Race: window-max (rust)
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

const WINDOWS: [usize; 5] = [10, 100, 1000, 10000, 100000];

fn window_max(values: &[i64]) -> String {
    let n = values.len();
    let mut dq = vec![0usize; n];
    let mut hashes = Vec::with_capacity(WINDOWS.len());
    for w in WINDOWS {
        let (mut head, mut tail) = (0, 0);
        let mut h = 0u32;
        for (i, &v) in values.iter().enumerate() {
            while tail > head && values[dq[tail - 1]] <= v {
                tail -= 1;
            }
            dq[tail] = i;
            tail += 1;
            if dq[head] + w <= i {
                head += 1;
            }
            if i + 1 >= w {
                h = h.wrapping_mul(31).wrapping_add(values[dq[head]] as u32);
            }
        }
        hashes.push(h.to_string());
    }
    format!("{n}:{}", hashes.join(":"))
}
