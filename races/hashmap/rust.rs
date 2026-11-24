// Race: hashmap (rust)
// Extracted from suite/rust/src/misc.rs — build and run via suite/run.py

use std::collections::HashMap;
use regex::Regex;
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use crate::inputs::{fnv1a, h31};

pub fn hashmap(inserts: &[i64], lookups: &[i64]) -> String {
    let mut counts: HashMap<i64, i64> = HashMap::new();
    for &k in inserts {
        *counts.entry(k).or_insert(0) += 1;
    }
    let hits = lookups.iter().filter(|k| counts.contains_key(k)).count();
    format!("{}:{hits}", counts.len())
}
