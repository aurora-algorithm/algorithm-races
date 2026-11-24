// Race: pi-digits (rust)
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

const CHUD_A: u64 = 13591409;

const CHUD_B: u64 = 545140134;

const CHUD_C3_24: u64 = 10939058860032000;

const DIGITS_PER_TERM: usize = 14;

const GUARD_DIGITS: u32 = 10;

/// P(a,b), Q(a,b), T(a,b) of the Chudnovsky terms a .. b-1.
fn binary_split(a: u64, b: u64) -> (BigInt, BigInt, BigInt) {
    if b - a == 1 {
        let (p, q) = if a == 0 {
            (BigInt::from(1), BigInt::from(1))
        } else {
            let p = BigInt::from((6 * a - 5) * (2 * a - 1) * (6 * a - 1));
            (p, BigInt::from(a).pow(3) * CHUD_C3_24)
        };
        let t = &p * (CHUD_A + CHUD_B * a);
        return (p, q, if a % 2 == 1 { -t } else { t });
    }
    let m = (a + b) / 2;
    let (p1, q1, t1) = binary_split(a, m);
    let (p2, q2, t2) = binary_split(m, b);
    (&p1 * &p2, q1 * &q2, t1 * q2 + p1 * t2)
}

/// Floor square root of n > 0 by precision doubling (CPython's `math.isqrt`), written out as in the others.
fn isqrt(n: &BigInt) -> BigInt {
    let c = (n.bits() - 1) / 2;
    let mut a = BigInt::from(1);
    let mut d = 0;
    for s in (0..u64::BITS - c.leading_zeros()).rev() {
        let e = d;
        d = c >> s;
        a = (&a << (d - e - 1)) + (n >> (2 * c - e - d + 1)) / &a;
    }
    if &a * &a > *n { a - 1 } else { a }
}

fn pi_digits(n: usize) -> String {
    let terms = (n / DIGITS_PER_TERM + 2) as u64;
    let (_p, q, t) = binary_split(0, terms);
    let one = BigInt::from(10).pow(n as u32 + GUARD_DIGITS);
    let sqrt_c = isqrt(&(BigInt::from(10005) * &one * &one));
    let pi = BigInt::from(426880) * sqrt_c * q / t;
    (pi / BigInt::from(10).pow(GUARD_DIGITS)).to_string()
}

fn pi_checksum(digits: &str) -> String {
    let last = &digits[digits.len().saturating_sub(50)..];
    let sum: u32 = digits.bytes().map(|d| u32::from(d - b'0')).sum();
    format!("{}:{last}:{sum}", digits.len() - 1)
}
