// Race: matmul (rust)
// Extracted from suite/rust/src/misc.rs — build and run via suite/run.py

use std::collections::HashMap;
use regex::Regex;
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use crate::inputs::{fnv1a, h31};

pub fn matmul(a: &[f64], b: &[f64], n: usize) -> Vec<f64> {
    let mut c = vec![0.0; n * n];
    for i in 0..n {
        let row = i * n;
        for j in 0..n {
            let mut s = 0.0;
            for k in 0..n {
                s += a[row + k] * b[k * n + j];
            }
            c[row + j] = s;
        }
    }
    c
}

pub fn matmul_checksum(c: &[f64]) -> String {
    let total: i64 = c.iter().map(|&v| v as i64).sum();
    format!("{total}:{}", h31(c.iter().map(|&v| v as i64)))
}
