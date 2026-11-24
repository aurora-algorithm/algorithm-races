// Race: ordered-map (rust)
// Extracted from suite/rust/src/algo.rs — build and run via suite/run.py

use std::cmp::{Ordering, Reverse};
use std::collections::{BTreeMap, BinaryHeap, HashMap, VecDeque};
use crate::inputs::{XorShift32, h31, hashmap_input, ints};
use crate::work::Outcome;
use crate::{Args, measure};

fn ordered_map(inserts: &[i64], lookups: &[i64]) -> String {
    let mut counts = BTreeMap::new();
    for &k in inserts {
        *counts.entry(k).or_insert(0) += 1;
    }
    let hits = lookups.iter().filter(|k| counts.contains_key(k)).count();
    format!("{}:{hits}:{}", counts.len(), h31(counts.keys().copied()))
}
