//! The ssearch kernels (../SPEC.md section 16): lower bound in small sorted arrays, a plain loop against binary
//! search, at every size from 4 to 1024. Kernel names are `ssearch.rust.WAY`.

use std::time::Instant;

use serde_json::{Map, Value};

use crate::Args;
use crate::inputs::XorShift32;
use crate::work::Outcome;

const SIZES: [usize; 17] = [
    4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024,
];

fn linear(a: &[i64], q: i64) -> usize {
    a.iter().position(|&x| x >= q).unwrap_or(a.len())
}

fn linear_count(a: &[i64], q: i64) -> usize {
    a.iter().filter(|&&x| x < q).count()
}

fn binary(a: &[i64], q: i64) -> usize {
    let (mut lo, mut hi) = (0, a.len());
    while lo < hi {
        let mid = (lo + hi) / 2;
        if a[mid] < q {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    lo
}

// The values are distinct, so the index binary_search returns for a hit is the lower bound.
fn binary_search(a: &[i64], q: i64) -> usize {
    a.binary_search(&q).unwrap_or_else(|i| i)
}

fn partition_point(a: &[i64], q: i64) -> usize {
    a.partition_point(|&x| x < q)
}

fn query_loop(a: &[i64], queries: &[i64], find: impl Fn(&[i64], i64) -> usize) -> (u64, u64) {
    let (mut hits, mut total) = (0u64, 0u64);
    for &q in queries {
        let lo = find(a, q);
        hits += u64::from(lo < a.len() && a[lo] == q);
        total += lo as u64;
    }
    (hits, total)
}

fn median(mut v: Vec<f64>) -> f64 {
    v.sort_by(f64::total_cmp);
    let m = v.len() / 2;
    if v.len() % 2 == 1 {
        v[m]
    } else {
        (v[m - 1] + v[m]) / 2.0
    }
}

fn sweep(args: &Args, find: impl Fn(&[i64], i64) -> usize + Copy) -> Outcome {
    let mut times = vec![0.0; args.repeats];
    let (mut hits, mut total) = (0u64, 0u64);
    let mut extra = Map::new();
    for n in SIZES {
        let mut rng = XorShift32::new();
        let mut v = 0i64;
        let a: Vec<i64> = (0..n)
            .map(|_| {
                v += 1 + i64::from(rng.next_u32() % 4);
                v
            })
            .collect();
        let bound = u32::try_from(v + 1).expect("the array's last value fits in 32 bits");
        let queries: Vec<i64> = (0..args.size)
            .map(|_| i64::from(rng.next_u32() % bound))
            .collect();
        let mut ns = Vec::with_capacity(args.repeats);
        let mut pass = (0, 0);
        for t in &mut times {
            let start = Instant::now();
            pass = query_loop(&a, &queries, find);
            let ms = start.elapsed().as_secs_f64() * 1e3;
            *t += ms;
            ns.push(ms * 1e6 / args.size as f64);
        }
        hits += pass.0;
        total += pass.1;
        let ns_median = (median(ns) * 1e3).round() / 1e3;
        extra.insert(format!("ns_{n}"), Value::from(ns_median));
    }
    let times = times.into_iter().map(|t| (t * 1e3).round() / 1e3).collect();
    (format!("{hits}:{total}"), times, Some(Value::Object(extra)))
}

pub fn run(args: &Args) -> Result<Option<Outcome>, String> {
    let Some(way) = args.kernel.strip_prefix("ssearch.rust.") else {
        return Ok(None);
    };
    let outcome = match way {
        "linear" => sweep(args, linear),
        "linear-count" => sweep(args, linear_count),
        "binary" => sweep(args, binary),
        "binary_search" => sweep(args, binary_search),
        "partition_point" => sweep(args, partition_point),
        _ => return Ok(None),
    };
    Ok(Some(outcome))
}
