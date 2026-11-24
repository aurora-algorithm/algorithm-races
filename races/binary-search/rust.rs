// Race: binary-search (rust)
// Extracted from suite/rust/src/algo.rs — build and run via suite/run.py

use std::cmp::{Ordering, Reverse};
use std::collections::{BTreeMap, BinaryHeap, HashMap, VecDeque};
use crate::inputs::{XorShift32, h31, hashmap_input, ints};
use crate::work::Outcome;
use crate::{Args, measure};

fn sorted_array(n: usize) -> (Vec<i64>, Vec<i64>) {
    let mut rng = XorShift32::new();
    let mut v = 0;
    let a: Vec<i64> = (0..n)
        .map(|_| {
            v += 1 + i64::from(rng.next_u32() % 4);
            v
        })
        .collect();
    let queries = (0..n)
        .map(|_| i64::from(rng.next_u32()) % (v + 1))
        .collect();
    (a, queries)
}

fn binary_search(a: &[i64], queries: &[i64]) -> String {
    let n = a.len();
    let (mut hits, mut total) = (0u64, 0u64);
    for &q in queries {
        let (mut lo, mut hi) = (0, n);
        while lo < hi {
            let mid = (lo + hi) / 2;
            if a[mid] < q {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        if lo < n && a[lo] == q {
            hits += 1;
        }
        total += lo as u64;
    }
    format!("{hits}:{total}")
}
