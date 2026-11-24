// Race: json (rust)
// Extracted from suite/rust/src/misc.rs — build and run via suite/run.py

use std::collections::HashMap;
use regex::Regex;
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use crate::inputs::{fnv1a, h31};

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
