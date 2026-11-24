// Race: dijkstra (rust)
// Extracted from suite/rust/src/algo.rs — build and run via suite/run.py

use std::cmp::{Ordering, Reverse};
use std::collections::{BTreeMap, BinaryHeap, HashMap, VecDeque};
use crate::inputs::{XorShift32, h31, hashmap_input, ints};
use crate::work::Outcome;
use crate::{Args, measure};

/// Four edges per node u at 4u..4u+3: one to u+1 (a ring, so every node is reachable), three random.
fn graph(n: usize) -> (Vec<usize>, Vec<u64>) {
    let mut rng = XorShift32::new();
    let mut to = vec![0; 4 * n];
    let mut weight = vec![0; 4 * n];
    for u in 0..n {
        to[4 * u] = (u + 1) % n;
        weight[4 * u] = 1 + u64::from(rng.next_u32() % 100);
        for e in 4 * u + 1..4 * u + 4 {
            to[e] = rng.next_u32() as usize % n;
            weight[e] = 1 + u64::from(rng.next_u32() % 100);
        }
    }
    (to, weight)
}

/// A heap entry ordered by distance only, as Go's and Goose's heaps compare it (a tuple would compare the node too).
struct Visit {
    dist: u64,
    node: usize,
}

impl PartialEq for Visit {
    fn eq(&self, other: &Self) -> bool {
        self.dist == other.dist
    }
}

impl Eq for Visit {}

impl PartialOrd for Visit {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for Visit {
    fn cmp(&self, other: &Self) -> Ordering {
        self.dist.cmp(&other.dist)
    }
}

fn dijkstra(n: usize, to: &[usize], weight: &[u64]) -> String {
    let mut dist = vec![u64::MAX; n];
    dist[0] = 0;
    let mut heap = BinaryHeap::from([Reverse(Visit { dist: 0, node: 0 })]);
    while let Some(Reverse(Visit { dist: d, node: u })) = heap.pop() {
        if d > dist[u] {
            continue;
        }
        for e in 4 * u..4 * u + 4 {
            let v = to[e];
            let nd = d + weight[e];
            if nd < dist[v] {
                dist[v] = nd;
                heap.push(Reverse(Visit { dist: nd, node: v }));
            }
        }
    }
    let reached: Vec<u64> = dist.into_iter().filter(|&d| d < u64::MAX).collect();
    let total: u64 = reached.iter().sum();
    let max = reached.iter().max().copied().unwrap_or(0);
    format!("{}:{total}:{max}", reached.len())
}
