// Race: bsort-int-reversed.goose.sort (rust)
// Extracted from suite/rust/src/ways.rs — build and run via suite/run.py

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
