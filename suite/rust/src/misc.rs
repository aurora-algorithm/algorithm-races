//! Library and loop kernels (SPEC.md section 5).

use std::collections::HashMap;

use regex::Regex;
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};

use crate::inputs::{fnv1a, h31};

pub const LOG_PATTERN: &str = r"(GET|POST) /api/v[0-9]/users/([0-9]+) (5[0-9][0-9])";

pub fn regex_log(pattern: &Regex, text: &str) -> String {
    let mut matches = 0;
    let mut ids: u64 = 0;
    for caps in pattern.captures_iter(text) {
        matches += 1;
        ids += caps[2].parse::<u64>().unwrap();
    }
    format!("{matches}:{ids}")
}

pub fn hashmap(inserts: &[i64], lookups: &[i64]) -> String {
    let mut counts: HashMap<i64, i64> = HashMap::new();
    for &k in inserts {
        *counts.entry(k).or_insert(0) += 1;
    }
    let hits = lookups.iter().filter(|k| counts.contains_key(k)).count();
    format!("{}:{hits}", counts.len())
}

pub fn sha256(buf: &[u8]) -> String {
    Sha256::digest(buf)
        .iter()
        .map(|b| format!("{b:02x}"))
        .collect()
}

pub fn sieve(n: usize) -> String {
    let mut composite = vec![false; n + 1];
    let mut i = 2;
    while i * i <= n {
        if !composite[i] {
            for j in (i * i..=n).step_by(i) {
                composite[j] = true;
            }
        }
        i += 1;
    }
    let primes = (2..=n).filter(|&i| !composite[i]).count();
    primes.to_string()
}

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

#[derive(Serialize, Deserialize)]
struct Document {
    orders: Vec<Order>,
}

#[derive(Serialize, Deserialize)]
struct Order {
    id: u64,
    customer_id: u64,
    status: String,
    total_cents: u64,
    items: u64,
    tags: Vec<String>,
}

pub struct JsonResult {
    orders: usize,
    total_cents: u64,
    items: u64,
    output: String,
}

/// Parses the whole document into typed structs, sums two fields, writes it back as compact JSON.
pub fn json_roundtrip(text: &str) -> JsonResult {
    let doc: Document = serde_json::from_str(text).unwrap();
    let total_cents = doc.orders.iter().map(|o| o.total_cents).sum();
    let items = doc.orders.iter().map(|o| o.items).sum();
    let output = serde_json::to_string(&doc).unwrap();
    JsonResult {
        orders: doc.orders.len(),
        total_cents,
        items,
        output,
    }
}

pub fn json_checksum(r: &JsonResult) -> String {
    let bytes = r.output.as_bytes();
    format!(
        "{}:{}:{}:{}:{}",
        r.orders,
        r.total_cents,
        r.items,
        bytes.len(),
        fnv1a(bytes)
    )
}
