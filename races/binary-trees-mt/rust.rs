// Race: binary-trees-mt (rust)
// Extracted from suite/rust/src/mt.rs — build and run via suite/run.py

use std::thread;
use crate::bg::{binary_trees_lines, depth_checks, mandelbrot_row, row_bytes};
use crate::sorts::{merge, merge_sort};

/// Each depth's iterations are split into T equal shares; the main thread sums the checks.
pub fn binary_trees_mt(n: u32, threads: usize) -> String {
    binary_trees_lines(n, |depth, iterations| {
        let share = iterations / threads as u64;
        thread::scope(|s| {
            let workers: Vec<_> = (0..threads)
                .map(|_| s.spawn(move || depth_checks(depth, share)))
                .collect();
            workers.into_iter().map(|w| w.join().unwrap()).sum()
        })
    })
}
