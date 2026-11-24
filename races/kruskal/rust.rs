// Race: kruskal (rust)
// Extracted from suite/rust/src/algo.rs — build and run via suite/run.py

use std::cmp::{Ordering, Reverse};
use std::collections::{BTreeMap, BinaryHeap, HashMap, VecDeque};
use crate::inputs::{XorShift32, h31, hashmap_input, ints};
use crate::work::Outcome;
use crate::{Args, measure};

#[derive(Clone, Copy)]
struct Edge {
    w: u32,
    u: usize,
    v: usize,
}

fn edge_list(n: usize) -> Vec<Edge> {
    let mut rng = XorShift32::new();
    (0..4 * n)
        .map(|_| {
            let u = rng.next_u32() as usize % n;
            let v = rng.next_u32() as usize % n;
            Edge {
                w: rng.next_u32() % 1_000_000,
                u,
                v,
            }
        })
        .collect()
}

fn find(parent: &mut [usize], mut x: usize) -> usize {
    let mut root = x;
    while parent[root] != root {
        root = parent[root];
    }
    while parent[x] != root {
        let next = parent[x];
        parent[x] = root;
        x = next;
    }
    root
}

fn kruskal(n: usize, mut edges: Vec<Edge>) -> String {
    edges.sort_unstable_by_key(|e| e.w);
    let mut parent: Vec<usize> = (0..n).collect();
    let mut rank = vec![0u8; n];
    let (mut count, mut total) = (0u64, 0u64);
    for e in &edges {
        let mut ru = find(&mut parent, e.u);
        let mut rv = find(&mut parent, e.v);
        if ru == rv {
            continue;
        }
        if rank[ru] < rank[rv] {
            std::mem::swap(&mut ru, &mut rv);
        }
        parent[rv] = ru;
        if rank[ru] == rank[rv] {
            rank[ru] += 1;
        }
        count += 1;
        total += u64::from(e.w);
    }
    format!("{count}:{total}")
}
