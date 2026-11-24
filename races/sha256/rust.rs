// Race: sha256 (rust)
// Extracted from suite/rust/src/misc.rs — build and run via suite/run.py

use std::collections::HashMap;
use regex::Regex;
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use crate::inputs::{fnv1a, h31};

pub fn sha256(buf: &[u8]) -> String {
    Sha256::digest(buf)
        .iter()
        .map(|b| format!("{b:02x}"))
        .collect()
}
