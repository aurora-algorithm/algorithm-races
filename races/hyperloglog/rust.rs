// Race: hyperloglog (rust)
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

/// MurmurHash3's 32-bit finalizer: the one hash every language uses for the probabilistic kernels.
fn fmix32(mut h: u32) -> u32 {
    h ^= h >> 16;
    h = h.wrapping_mul(0x85eb_ca6b);
    h ^= h >> 13;
    h = h.wrapping_mul(0xc2b2_ae35);
    h ^= h >> 16;
    h
}

const HLL_P: u32 = 14;

const HLL_M: usize = 1 << HLL_P;

fn hyperloglog(n: usize) -> String {
    let mut rng = XorShift32::new();
    let universe = (n / 4).max(1) as u32;
    let mut registers = vec![0u8; HLL_M];
    for _ in 0..n {
        let h = fmix32(rng.next_u32() % universe);
        let w = h << HLL_P;
        let rank = if w == 0 {
            32 - HLL_P + 1
        } else {
            w.leading_zeros() + 1
        };
        let idx = (h >> (32 - HLL_P)) as usize;
        registers[idx] = registers[idx].max(rank as u8);
    }
    let mut z = 0.0;
    let mut zeros = 0;
    for &r in &registers {
        z += 2f64.powi(-i32::from(r));
        zeros += usize::from(r == 0);
    }
    let m = HLL_M as f64;
    let mut estimate = 0.7213 / (1.0 + 1.079 / m) * m * m / z;
    if estimate <= 2.5 * m && zeros > 0 {
        estimate = m * (m / zeros as f64).ln();
    }
    let registers = registers.iter().map(|&r| i64::from(r));
    format!("{n}:{}:{}", (estimate + 0.5).floor(), h31(registers))
}
