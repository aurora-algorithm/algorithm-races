// Race: mandelbrot-mt (rust)
// Extracted from suite/rust/src/mt.rs — build and run via suite/run.py

use std::thread;
use crate::bg::{binary_trees_lines, depth_checks, mandelbrot_row, row_bytes};
use crate::sorts::{merge, merge_sort};

/// Thread t computes rows t, t + T, t + 2T, ... of the shared bitmap.
pub fn mandelbrot_mt(n: usize, threads: usize) -> Vec<u8> {
    let mut bitmap = vec![0u8; n * row_bytes(n)];
    let mut shares: Vec<Vec<(usize, &mut [u8])>> = (0..threads).map(|_| Vec::new()).collect();
    for (y, row) in bitmap.chunks_mut(row_bytes(n)).enumerate() {
        shares[y % threads].push((y, row));
    }
    thread::scope(|s| {
        for share in shares {
            s.spawn(move || {
                for (y, row) in share {
                    mandelbrot_row(n, y, row);
                }
            });
        }
    });
    bitmap
}
