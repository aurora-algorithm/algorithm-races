//! CPU kernels, Rust version (see ../SPEC.md).
//!
//! usage: kernels KERNEL --size N [--repeats R] [--threads T] [--data DIR]
//! Prints one JSON line: kernel, size, threads, checksum, times_ms (kernel only), runtime.

mod algo;
mod arena;
mod autocomplete;
mod bg;
mod inputs;
mod misc;
mod mt;
mod puzzle;
mod puzzle_text;
mod sorts;
mod ssearch;
mod ways;
mod ways_words;
mod work;

use std::hint::black_box;
use std::path::PathBuf;
use std::process::ExitCode;
use std::time::Instant;

use regex::Regex;
use serde::Serialize;

use crate::inputs::{fnv1a, h31};

struct Args {
    kernel: String,
    size: usize,
    repeats: usize,
    threads: usize,
    data: PathBuf,
}

#[derive(Serialize)]
struct Row<'a> {
    kernel: &'a str,
    size: usize,
    threads: usize,
    checksum: String,
    times_ms: Vec<f64>,
    runtime: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    extra: Option<serde_json::Value>,
}

fn parse_args() -> Result<Args, String> {
    let mut argv = std::env::args().skip(1);
    let kernel = argv.next().ok_or("missing KERNEL")?;
    let mut args = Args {
        kernel,
        size: 0,
        repeats: 1,
        threads: 1,
        data: PathBuf::from("."),
    };
    let mut size = None;
    while let Some(flag) = argv.next() {
        let value = argv.next().ok_or(format!("{flag} needs a value"))?;
        let number = || {
            value
                .parse::<usize>()
                .ok()
                .filter(|&v| v > 0)
                .ok_or(format!("{flag}: expected a positive integer, got {value}"))
        };
        match flag.as_str() {
            "--size" => size = Some(number()?),
            "--repeats" => args.repeats = number()?,
            "--threads" => args.threads = number()?,
            "--data" => args.data = PathBuf::from(&value),
            _ => return Err(format!("unknown option {flag}")),
        }
    }
    args.size = size.ok_or("--size is required")?;
    Ok(args)
}

fn elapsed_ms(start: Instant) -> f64 {
    let ms = start.elapsed().as_nanos() as f64 / 1e6;
    (ms * 1000.0).round() / 1000.0
}

/// Runs `kernel` once per repeat on a fresh input from `fresh` (made outside the timed region).
/// Returns the last repeat's result and every repeat's time in milliseconds.
fn measure<I, R>(
    repeats: usize,
    mut fresh: impl FnMut() -> I,
    mut kernel: impl FnMut(I) -> R,
) -> (R, Vec<f64>) {
    let mut times = Vec::with_capacity(repeats);
    let mut last = None;
    for _ in 0..repeats {
        let input = fresh();
        let start = Instant::now();
        let result = black_box(kernel(black_box(input)));
        times.push(elapsed_ms(start));
        last = Some(result);
    }
    (last.expect("repeats is at least 1"), times)
}

fn sorted_checksum(a: &[i64]) -> String {
    h31(a.iter().copied()).to_string()
}

fn sort_kernel(args: &Args, sort: impl FnMut(Vec<i64>) -> Vec<i64>) -> (String, Vec<f64>) {
    let input = inputs::ints(args.size);
    let (sorted, times) = measure(args.repeats, || input.clone(), sort);
    (sorted_checksum(&sorted), times)
}

fn tree_depth(size: usize) -> Result<u32, String> {
    u32::try_from(size).map_err(|_| format!("--size {size} is too deep for a tree"))
}

fn run_kernel(args: &Args) -> Result<Option<(String, Vec<f64>)>, String> {
    let (n, repeats, threads) = (args.size, args.repeats, args.threads);
    let result = match args.kernel.as_str() {
        "sort-int" => sort_kernel(args, sorts::std_sort),
        "quicksort" => sort_kernel(args, sorts::run_quicksort),
        "mergesort" => sort_kernel(args, sorts::run_mergesort),
        "heapsort" => sort_kernel(args, sorts::run_heapsort),
        "radixsort" => sort_kernel(args, sorts::radix_sort),
        "mergesort-mt" => sort_kernel(args, |a| mt::mergesort_mt(a, threads)),
        "sort-str" => {
            let input = inputs::strings(n);
            let (sorted, times) = measure(repeats, || input.clone(), sorts::std_sort_strings);
            let text: String = sorted.iter().map(|s| format!("{s}\n")).collect();
            (fnv1a(text.as_bytes()).to_string(), times)
        }
        "binary-trees" => {
            let depth = tree_depth(n)?;
            let (text, times) = measure(repeats, || depth, bg::binary_trees);
            (fnv1a(text.as_bytes()).to_string(), times)
        }
        "binary-trees-mt" => {
            let depth = tree_depth(n)?;
            let (text, times) = measure(repeats, || depth, |d| mt::binary_trees_mt(d, threads));
            (fnv1a(text.as_bytes()).to_string(), times)
        }
        "nbody" => measure(repeats, || n, bg::nbody),
        "mandelbrot" => {
            let (bitmap, times) = measure(repeats, || n, bg::mandelbrot);
            (fnv1a(&bitmap).to_string(), times)
        }
        "mandelbrot-mt" => {
            let (bitmap, times) = measure(repeats, || n, |n| mt::mandelbrot_mt(n, threads));
            (fnv1a(&bitmap).to_string(), times)
        }
        "fannkuch" => measure(repeats, || n, bg::fannkuch),
        "spectral-norm" => measure(repeats, || n, bg::spectral_norm),
        "regex-log" => {
            let text = inputs::log_text(n);
            let pattern = Regex::new(misc::LOG_PATTERN).map_err(|e| e.to_string())?;
            measure(repeats, || &text, |t| misc::regex_log(&pattern, t))
        }
        "hashmap" => {
            let (inserts, lookups) = inputs::hashmap_input(n);
            measure(
                repeats,
                || (&inserts, &lookups),
                |(i, l)| misc::hashmap(i, l),
            )
        }
        "sha256" => {
            let buf = inputs::sha_buffer(n);
            measure(repeats, || &buf, misc::sha256)
        }
        "sieve" => measure(repeats, || n, misc::sieve),
        "matmul" => {
            let (a, b) = inputs::matrices(n);
            let (c, times) = measure(repeats, || (&a, &b), |(a, b)| misc::matmul(a, b, n));
            (misc::matmul_checksum(&c), times)
        }
        "json" => {
            let path = args.data.join(format!("orders-{n}.json"));
            let text = std::fs::read_to_string(&path)
                .map_err(|e| format!("cannot read {}: {e}", path.display()))?;
            let (result, times) = measure(repeats, || &text, |t| misc::json_roundtrip(t));
            (misc::json_checksum(&result), times)
        }
        _ => return Ok(None),
    };
    Ok(Some(result))
}

fn runtime(kernel: &str) -> String {
    let profile = if cfg!(debug_assertions) {
        "debug"
    } else {
        "release"
    };
    let rustc = env!("RUSTC_VERSION");
    if work::is_work(kernel) {
        format!("{rustc} ({profile}) {}", env!("WORK_CRATES"))
    } else if kernel.contains(".rust.") {
        format!("{rustc} ({profile}) {}", env!("BUILTINS_CRATES"))
    } else {
        format!("{rustc} ({profile})")
    }
}

fn run_any(args: &Args) -> Result<Option<work::Outcome>, String> {
    if work::is_work(&args.kernel) {
        return work::run(args);
    }
    if let Some(outcome) = algo::run(args)? {
        return Ok(Some(outcome));
    }
    if let Some(outcome) = puzzle::run(args)? {
        return Ok(Some(outcome));
    }
    if let Some(outcome) = arena::run(args)? {
        return Ok(Some(outcome));
    }
    if let Some(outcome) = autocomplete::run(args)? {
        return Ok(Some(outcome));
    }
    if let Some(outcome) = ways::run(args)? {
        return Ok(Some(outcome));
    }
    if let Some(outcome) = ways_words::run(args)? {
        return Ok(Some(outcome));
    }
    if let Some(outcome) = ssearch::run(args)? {
        return Ok(Some(outcome));
    }
    Ok(run_kernel(args)?.map(|(checksum, times)| (checksum, times, None)))
}

fn main() -> ExitCode {
    let args = match parse_args() {
        Ok(args) => args,
        Err(e) => {
            eprintln!("error: {e}");
            eprintln!("usage: kernels KERNEL --size N [--repeats R] [--threads T] [--data DIR]");
            return ExitCode::from(2);
        }
    };
    let (checksum, times_ms, extra) = match run_any(&args) {
        Ok(Some(result)) => result,
        Ok(None) => {
            eprintln!("unknown kernel {}", args.kernel);
            return ExitCode::from(2);
        }
        Err(e) => {
            eprintln!("error: {e}");
            return ExitCode::FAILURE;
        }
    };
    let row = Row {
        kernel: &args.kernel,
        size: args.size,
        threads: args.threads,
        checksum,
        times_ms,
        runtime: runtime(&args.kernel),
        extra,
    };
    println!(
        "{}",
        serde_json::to_string(&row).expect("a row always serialises")
    );
    ExitCode::SUCCESS
}
