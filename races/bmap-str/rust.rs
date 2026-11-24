// Race: bmap-str.go.map (rust)
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

/// Keys are `&str` borrowed from the input, as the other languages store references to existing strings. Each
/// repeat gets its own copies of the inserts and, separately, of the lookups (made untimed), as in the languages
/// whose strings cache their hash; the run hands them back so they are freed after the timer stops.
fn map_strings(args: &Args, way: &str) -> Option<Outcome> {
    let inserts = strings(args.size);
    let lookups: Vec<&str> = inserts
        .iter()
        .enumerate()
        .map(|(i, k)| if i % 2 == 0 { k } else { &k[..k.len() - 1] })
        .collect();
    let run: StrCount = match way {
        "std.HashMap" => |ins, look| count!(HashMap::new(), keys(ins), keys(look)),
        "hashbrown.HashMap" => |ins, look| count!(hashbrown::HashMap::new(), keys(ins), keys(look)),
        "ahash.AHashMap" => |ins, look| count!(ahash::AHashMap::new(), keys(ins), keys(look)),
        "rustc_hash.FxHashMap" => {
            |ins, look| count!(rustc_hash::FxHashMap::default(), keys(ins), keys(look))
        }
        _ => return None,
    };
    let fresh = || {
        (
            owned(inserts.iter().map(String::as_str)),
            owned(lookups.iter().copied()),
        )
    };
    let ((checksum, _), times) = measure(args.repeats, fresh, |(ins, look)| {
        let checksum = run(&ins, &look);
        (checksum, (ins, look))
    });
    Some((checksum, times, None))
}

type StrCount = fn(&[String], &[String]) -> String;
