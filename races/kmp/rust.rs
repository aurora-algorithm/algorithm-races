// Race: kmp (rust)
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

fn kmp(t: &str, pattern: &str) -> Matches {
    let (t, p) = (t.as_bytes(), pattern.as_bytes());
    let m = p.len();
    let mut fail = vec![0; m];
    let mut k = 0;
    for i in 1..m {
        while k > 0 && p[i] != p[k] {
            k = fail[k - 1];
        }
        if p[i] == p[k] {
            k += 1;
        }
        fail[i] = k;
    }
    let mut found = Matches::new();
    k = 0;
    for (i, &ch) in t.iter().enumerate() {
        while k > 0 && ch != p[k] {
            k = fail[k - 1];
        }
        if ch == p[k] {
            k += 1;
        }
        if k == m {
            found.add(i + 1 - m);
            k = fail[k - 1];
        }
    }
    found
}
