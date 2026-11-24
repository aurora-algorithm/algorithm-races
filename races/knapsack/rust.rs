// Race: knapsack (rust)
// Extracted from suite/rust/src/algo.rs — build and run via suite/run.py

use std::cmp::{Ordering, Reverse};
use std::collections::{BTreeMap, BinaryHeap, HashMap, VecDeque};
use crate::inputs::{XorShift32, h31, hashmap_input, ints};
use crate::work::Outcome;
use crate::{Args, measure};

fn items() -> Vec<(usize, u64)> {
    let mut rng = XorShift32::new();
    (0..KNAPSACK_ITEMS)
        .map(|_| {
            let weight = 1 + (rng.next_u32() % 1000) as usize;
            (weight, 1 + u64::from(rng.next_u32() % 1000))
        })
        .collect()
}

fn knapsack(capacity: usize, things: &[(usize, u64)]) -> String {
    let mut best = vec![0u64; capacity + 1];
    for &(weight, value) in things {
        for c in (weight..capacity + 1).rev() {
            if best[c - weight] + value > best[c] {
                best[c] = best[c - weight] + value;
            }
        }
    }
    format!("{}:{}", best[capacity], h31(best.iter().map(|&v| v as i64)))
}
