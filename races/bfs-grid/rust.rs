// Race: bfs-grid (rust)
// Extracted from suite/rust/src/algo.rs — build and run via suite/run.py

use std::cmp::{Ordering, Reverse};
use std::collections::{BTreeMap, BinaryHeap, HashMap, VecDeque};
use crate::inputs::{XorShift32, h31, hashmap_input, ints};
use crate::work::Outcome;
use crate::{Args, measure};

fn grid(n: usize) -> Vec<bool> {
    let mut rng = XorShift32::new();
    let mut walls: Vec<bool> = (0..n * n).map(|_| rng.next_u32() % 100 < 30).collect();
    for cell in [0, 1, n, n * n - n - 1, n * n - 2, n * n - 1] {
        walls[cell] = false;
    }
    walls
}

fn bfs_grid(n: usize, walls: &[bool]) -> String {
    let mut dist = vec![-1i32; n * n];
    dist[0] = 0;
    let mut queue = VecDeque::from([0usize]);
    while let Some(cell) = queue.pop_front() {
        let d = dist[cell] + 1;
        let (row, col) = (cell / n, cell % n);
        let neighbours = [
            (cell.wrapping_sub(n), row > 0),
            (cell + n, row < n - 1),
            (cell.wrapping_sub(1), col > 0),
            (cell + 1, col < n - 1),
        ];
        for (next, ok) in neighbours {
            if ok && !walls[next] && dist[next] < 0 {
                dist[next] = d;
                queue.push_back(next);
            }
        }
    }
    let reached = dist.iter().filter(|&&d| d >= 0).count();
    let total: i64 = dist
        .iter()
        .filter(|&&d| d >= 0)
        .map(|&d| i64::from(d))
        .sum();
    format!("{reached}:{}:{total}", dist[n * n - 1])
}
