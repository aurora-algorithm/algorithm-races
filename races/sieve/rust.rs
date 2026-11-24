// Race: sieve (rust)
// Extracted from suite/rust/src/misc.rs — build and run via suite/run.py

use std::collections::HashMap;
use regex::Regex;
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use crate::inputs::{fnv1a, h31};

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
