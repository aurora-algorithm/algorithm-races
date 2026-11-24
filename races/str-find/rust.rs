// Race: str-find (rust)
// Extracted from suite/rust/src/algo.rs — build and run via suite/run.py

use std::cmp::{Ordering, Reverse};
use std::collections::{BTreeMap, BinaryHeap, HashMap, VecDeque};
use crate::inputs::{XorShift32, h31, hashmap_input, ints};
use crate::work::Outcome;
use crate::{Args, measure};

const ACGT: &[u8; 4] = b"acgt";

const PATTERN_LEN: usize = 8;

fn dna(length: usize, count: usize) -> Vec<Vec<u8>> {
    let mut rng = XorShift32::new();
    (0..count)
        .map(|_| {
            (0..length)
                .map(|_| ACGT[(rng.next_u32() % 4) as usize])
                .collect()
        })
        .collect()
}

fn text(n: usize) -> (String, String) {
    let bytes = dna(n, 1).remove(0);
    let t = String::from_utf8(bytes).expect("acgt is ASCII");
    let p = t[n / 2..n / 2 + PATTERN_LEN].to_string();
    (t, p)
}

struct Matches {
    count: usize,
    first: i64,
    last: i64,
}

impl Matches {
    fn new() -> Self {
        Matches {
            count: 0,
            first: -1,
            last: -1,
        }
    }

    fn add(&mut self, pos: usize) {
        let pos = pos as i64;
        if self.first < 0 {
            self.first = pos;
        }
        self.last = pos;
        self.count += 1;
    }
}

impl std::fmt::Display for Matches {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}:{}:{}", self.count, self.first, self.last)
    }
}

fn str_find(t: &str, pattern: &str) -> Matches {
    let mut found = Matches::new();
    let mut start = 0;
    while let Some(offset) = t[start..].find(pattern) {
        found.add(start + offset);
        start += offset + 1;
    }
    found
}
