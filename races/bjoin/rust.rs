// Race: bjoin.go.strings.Join (rust)
// Extracted from suite/rust/src/ways.rs — build and run via suite/run.py

use std::collections::HashMap;
use rayon::ThreadPool;
use rayon::slice::ParallelSliceMut;
use serde_json::json;
use voracious_radix_sort::RadixSort;
use crate::inputs::{fnv1a, h31, ints, ints64, strings};
use crate::work::Outcome;
use crate::{Args, measure};

fn push_str(parts: &[String]) -> String {
    let mut out = String::new();
    for (i, part) in parts.iter().enumerate() {
        if i > 0 {
            out.push(',');
        }
        out.push_str(part);
    }
    out
}

fn with_capacity(parts: &[String]) -> String {
    let total = parts.iter().map(String::len).sum::<usize>() + parts.len().saturating_sub(1);
    let mut out = String::with_capacity(total);
    for (i, part) in parts.iter().enumerate() {
        if i > 0 {
            out.push(',');
        }
        out.push_str(part);
    }
    out
}

fn join(args: &Args, way: &str) -> Option<Outcome> {
    let build: fn(&[String]) -> String = match way {
        "slice.join" => |parts| parts.join(","),
        "String.push_str" => push_str,
        "String.with_capacity" => with_capacity,
        _ => return None,
    };
    let parts: Vec<String> = ints(args.size).iter().map(i64::to_string).collect();
    let (joined, times) = measure(args.repeats, || &parts, |p| build(p));
    let checksum = format!("{}:{}", joined.len(), fnv1a(joined.as_bytes()));
    Some((checksum, times, None))
}
