// Race: edit-distance (rust)
// Extracted from suite/rust/src/algo.rs — build and run via suite/run.py

use std::cmp::{Ordering, Reverse};
use std::collections::{BTreeMap, BinaryHeap, HashMap, VecDeque};
use crate::inputs::{XorShift32, h31, hashmap_input, ints};
use crate::work::Outcome;
use crate::{Args, measure};

const ACGT: &[u8; 4] = b"acgt";

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

fn edit_distance(a: &[u8], b: &[u8]) -> String {
    let mut prev: Vec<usize> = (0..=b.len()).collect();
    let mut cur = vec![0; b.len() + 1];
    for (i, &ca) in a.iter().enumerate() {
        cur[0] = i + 1;
        for (j, &cb) in b.iter().enumerate() {
            let cost = usize::from(ca != cb);
            cur[j + 1] = (prev[j + 1] + 1).min(cur[j] + 1).min(prev[j] + cost);
        }
        std::mem::swap(&mut prev, &mut cur);
    }
    prev[b.len()].to_string()
}
