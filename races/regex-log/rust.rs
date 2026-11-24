// Race: regex-log (rust)
// Extracted from suite/rust/src/misc.rs — build and run via suite/run.py

use std::collections::HashMap;
use regex::Regex;
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use crate::inputs::{fnv1a, h31};

pub fn regex_log(pattern: &Regex, text: &str) -> String {
    let mut matches = 0;
    let mut ids: u64 = 0;
    for caps in pattern.captures_iter(text) {
        matches += 1;
        ids += caps[2].parse::<u64>().unwrap();
    }
    format!("{matches}:{ids}")
}
