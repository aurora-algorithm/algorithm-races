// Race: mandelbrot (rust)
// Extracted from suite/rust/src/bg.rs — build and run via suite/run.py

use std::f64::consts::PI;
use std::fmt::Write;

/// One bitmap row: bit x (MSB first) is set when point (x, y) stays in the set for 50 iterations.
pub fn mandelbrot_row(n: usize, y: usize, row: &mut [u8]) {
    let ci = 2.0 * y as f64 / n as f64 - 1.0;
    for x in 0..n {
        let cr = 2.0 * x as f64 / n as f64 - 1.5;
        let (mut zr, mut zi, mut tr, mut ti) = (0.0, 0.0, 0.0, 0.0);
        let mut i = 0;
        while i < 50 && tr + ti <= 4.0 {
            zi = 2.0 * zr * zi + ci;
            zr = tr - ti + cr;
            tr = zr * zr;
            ti = zi * zi;
            i += 1;
        }
        if tr + ti <= 4.0 {
            row[x >> 3] |= 0x80 >> (x & 7);
        }
    }
}

pub fn row_bytes(n: usize) -> usize {
    n.div_ceil(8)
}

pub fn mandelbrot(n: usize) -> Vec<u8> {
    let mut bitmap = vec![0u8; n * row_bytes(n)];
    for (y, row) in bitmap.chunks_mut(row_bytes(n)).enumerate() {
        mandelbrot_row(n, y, row);
    }
    bitmap
}
