//! The builtins group (../builtins-NOTES.md): the standard library's ways of doing one job, plus the popular crates
//! that replace it, one way per kernel named `JOB.rust.WAY`. Every way of a job gets the same input and prints the
//! same checksum; only the call that does the job differs.

use std::collections::HashMap;

use rayon::ThreadPool;
use rayon::slice::ParallelSliceMut;
use serde_json::json;
use voracious_radix_sort::RadixSort;

use crate::inputs::{fnv1a, h31, ints, ints64, strings};
use crate::work::Outcome;
use crate::{Args, measure};

type Sorter<T> = Box<dyn Fn(&mut [T])>;
/// A sort, and the size of the thread pool it runs on (None: sequential).
type Way<T> = (Sorter<T>, Option<usize>);

pub fn run(args: &Args) -> Result<Option<Outcome>, String> {
    let Some((job, way)) = args.kernel.split_once(".rust.") else {
        return Ok(None);
    };
    match job {
        "bsort-int" | "bsort-int-sorted" | "bsort-int-reversed" | "bsort-int64" => sort_ints(args, job, way),
        "bsort-str" | "bsort-str-sorted" | "bsort-str-reversed" => sort_strings(args, job, way),
        "bmap-int" => Ok(map_ints(args, way)),
        "bmap-str" => Ok(map_strings(args, way)),
        "bjoin" => Ok(join(args, way)),
        _ => Ok(None),
    }
}

/// random, already sorted or descending: the input order is the only difference between the three sort jobs.
fn arrange<T: Ord>(mut a: Vec<T>, job: &str) -> Vec<T> {
    if job.ends_with("-sorted") {
        a.sort();
    } else if job.ends_with("-reversed") {
        a.sort_by(|x, y| y.cmp(x));
    }
    a
}

fn pool(threads: usize) -> Result<ThreadPool, String> {
    rayon::ThreadPoolBuilder::new()
        .num_threads(threads)
        .build()
        .map_err(|e| e.to_string())
}

/// The ways every element type has; the rayon pool is built here, before timing. Also returns the pool size.
fn common_sorter<T: Ord + Send + 'static>(
    way: &str,
    threads: usize,
) -> Result<Option<Way<T>>, String> {
    let sorter: Way<T> = match way {
        "sort" => (Box::new(|a| a.sort()), None),
        "sort_unstable" => (Box::new(|a| a.sort_unstable()), None),
        "rayon.par_sort" => {
            let p = pool(threads)?;
            let size = p.current_num_threads();
            (Box::new(move |a| p.install(|| a.par_sort())), Some(size))
        }
        "rayon.par_sort_unstable" => {
            let p = pool(threads)?;
            let size = p.current_num_threads();
            (
                Box::new(move |a| p.install(|| a.par_sort_unstable())),
                Some(size),
            )
        }
        _ => return Ok(None),
    };
    Ok(Some(sorter))
}

fn pool_extra(size: Option<usize>) -> Option<serde_json::Value> {
    size.map(|n| json!({"pool_threads": n}))
}

fn sort_ints(args: &Args, job: &str, way: &str) -> Result<Option<Outcome>, String> {
    let threads = args.threads;
    let (sorter, pool_size): Way<i64> = match way {
        "voracious_sort" => (Box::new(|a| a.voracious_sort()), None),
        // The crate builds its own rayon pool of `threads` inside the call, so that is timed.
        "voracious_mt_sort" => (
            Box::new(move |a| a.voracious_mt_sort(threads)),
            Some(threads),
        ),
        _ => match common_sorter(way, threads)? {
            Some(s) => s,
            None => return Ok(None),
        },
    };
    let input = if job == "bsort-int64" {
        ints64(args.size)
    } else {
        arrange(ints(args.size), job)
    };
    let (sorted, times) = measure(
        args.repeats,
        || input.clone(),
        |mut a| {
            sorter(&mut a);
            a
        },
    );
    Ok(Some((
        h31(sorted.iter().copied()).to_string(),
        times,
        pool_extra(pool_size),
    )))
}

fn sort_strings(args: &Args, job: &str, way: &str) -> Result<Option<Outcome>, String> {
    let Some((sorter, pool_size)) = common_sorter::<String>(way, args.threads)? else {
        return Ok(None);
    };
    let input = arrange(strings(args.size), job);
    let (sorted, times) = measure(
        args.repeats,
        || input.clone(),
        |mut a| {
            sorter(&mut a);
            a
        },
    );
    let text: String = sorted.iter().map(|s| format!("{s}\n")).collect();
    Ok(Some((
        fnv1a(text.as_bytes()).to_string(),
        times,
        pool_extra(pool_size),
    )))
}

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

fn owned<'a>(keys: impl Iterator<Item = &'a str>) -> Vec<String> {
    keys.map(str::to_owned).collect()
}

fn keys(strings: &[String]) -> impl Iterator<Item = &str> {
    strings.iter().map(String::as_str)
}

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
