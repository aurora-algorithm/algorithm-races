// Race: bloom (rust)
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

const BLOOM_BITS_PER_KEY: usize = 10;

fn bloom(keys: &[i64], lookups: &[i64]) -> String {
    let m = BLOOM_BITS_PER_KEY * keys.len();
    let mut bits = vec![0u8; m.div_ceil(8)];
    let m = m as u32;
    for &key in keys {
        for b in bloom_bits(key, m) {
            bits[b >> 3] |= 1 << (b & 7);
        }
    }
    let false_positives = lookups
        .iter()
        .filter(|&&key| bloom_bits(key, m).all(|b| bits[b >> 3] & (1 << (b & 7)) != 0))
        .count();
    let set_bits: u32 = bits.iter().map(|byte| byte.count_ones()).sum();
    format!("{}:{set_bits}:{false_positives}", keys.len())
}
