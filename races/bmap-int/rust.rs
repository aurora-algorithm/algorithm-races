// Race: bmap-int.go.map (rust)
// Extracted from suite/rust/src/ways.rs — build and run via suite/run.py

use std::collections::HashMap;
use rayon::ThreadPool;
use rayon::slice::ParallelSliceMut;
use serde_json::json;
use voracious_radix_sort::RadixSort;
use crate::inputs::{fnv1a, h31, ints, ints64, strings};
use crate::work::Outcome;
use crate::{Args, measure};

/// `m[k] += 1` for every insert (1 when absent), then how many lookups are keys; the same code for every map type.
macro_rules! count {
    ($map:expr, $inserts:expr, $lookups:expr) => {{
        let mut counts = $map;
        for k in $inserts {
            *counts.entry(k).or_insert(0u64) += 1;
        }
        let hits = $lookups.filter(|k| counts.contains_key(k)).count();
        format!("{}:{hits}", counts.len())
    }};
}

fn map_ints(args: &Args, way: &str) -> Option<Outcome> {
    let inserts = ints(args.size);
    let lookups: Vec<i64> = inserts
        .iter()
        .enumerate()
        .map(|(i, &k)| k ^ (i as i64 & 1))
        .collect();
    let ins = || inserts.iter().copied();
    let look = || lookups.iter().copied();
    let run: Box<dyn Fn() -> String> = match way {
        "std.HashMap" => Box::new(|| count!(HashMap::new(), ins(), look())),
        "hashbrown.HashMap" => Box::new(|| count!(hashbrown::HashMap::new(), ins(), look())),
        "ahash.AHashMap" => Box::new(|| count!(ahash::AHashMap::new(), ins(), look())),
        "rustc_hash.FxHashMap" => {
            Box::new(|| count!(rustc_hash::FxHashMap::default(), ins(), look()))
        }
        _ => return None,
    };
    let (checksum, times) = measure(args.repeats, || (), |()| run());
    Some((checksum, times, None))
}
