//! Multi-threaded kernels (SPEC.md section 6) on scoped threads.

use std::thread;

use crate::bg::{binary_trees_lines, depth_checks, mandelbrot_row, row_bytes};
use crate::sorts::{merge, merge_sort};

/// Cuts `slice` into consecutive pieces at `bounds` (bounds[0] = 0, last = slice length).
fn split_at_bounds<'a>(mut rest: &'a mut [i64], bounds: &[usize]) -> Vec<&'a mut [i64]> {
    let mut pieces = Vec::with_capacity(bounds.len() - 1);
    for w in bounds.windows(2) {
        let (piece, tail) = std::mem::take(&mut rest).split_at_mut(w[1] - w[0]);
        pieces.push(piece);
        rest = tail;
    }
    pieces
}

/// T threads each mergesort one chunk, then rounds of pairwise merges of neighbouring runs
/// (one thread per merge) until one run is left. `threads` must be a power of two.
pub fn mergesort_mt(mut a: Vec<i64>, threads: usize) -> Vec<i64> {
    assert!(threads.is_power_of_two(), "threads must be a power of two");
    let n = a.len();
    let mut aux = vec![0i64; n];
    let mut bounds: Vec<usize> = (0..=threads).map(|t| t * n / threads).collect();

    thread::scope(|s| {
        let chunks = split_at_bounds(&mut a, &bounds);
        let aux_chunks = split_at_bounds(&mut aux, &bounds);
        for (chunk, aux_chunk) in chunks.into_iter().zip(aux_chunks) {
            s.spawn(move || {
                let len = chunk.len();
                merge_sort(chunk, aux_chunk, 0, len);
            });
        }
    });

    while bounds.len() > 2 {
        let merged: Vec<usize> = bounds.iter().copied().step_by(2).collect();
        thread::scope(|s| {
            let runs = split_at_bounds(&mut a, &merged);
            let aux_runs = split_at_bounds(&mut aux, &merged);
            let pairs = bounds.windows(3).step_by(2);
            for ((run, aux_run), pair) in runs.into_iter().zip(aux_runs).zip(pairs) {
                let mid = pair[1] - pair[0];
                s.spawn(move || {
                    let len = run.len();
                    merge(run, aux_run, 0, mid, len);
                });
            }
        });
        bounds = merged;
    }
    a
}

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
