// Race: spectral-norm (rust)
// Extracted from suite/rust/src/bg.rs — build and run via suite/run.py

use std::f64::consts::PI;
use std::fmt::Write;

fn eval_a(i: usize, j: usize) -> f64 {
    1.0 / ((i + j) * (i + j + 1) / 2 + i + 1) as f64
}

fn mul_av(u: &[f64]) -> Vec<f64> {
    let n = u.len();
    (0..n)
        .map(|i| {
            let mut s = 0.0;
            for (j, uj) in u.iter().enumerate() {
                s += eval_a(i, j) * uj;
            }
            s
        })
        .collect()
}

fn mul_atv(u: &[f64]) -> Vec<f64> {
    let n = u.len();
    (0..n)
        .map(|i| {
            let mut s = 0.0;
            for (j, uj) in u.iter().enumerate() {
                s += eval_a(j, i) * uj;
            }
            s
        })
        .collect()
}

fn mul_atav(u: &[f64]) -> Vec<f64> {
    mul_atv(&mul_av(u))
}

pub fn spectral_norm(n: usize) -> String {
    let mut u = vec![1.0; n];
    let mut v = Vec::new();
    for _ in 0..10 {
        v = mul_atav(&u);
        u = mul_atav(&v);
    }
    let (mut vbv, mut vv) = (0.0, 0.0);
    for (ui, vi) in u.iter().zip(&v) {
        vbv += ui * vi;
        vv += vi * vi;
    }
    format!("{:.9}", (vbv / vv).sqrt())
}
