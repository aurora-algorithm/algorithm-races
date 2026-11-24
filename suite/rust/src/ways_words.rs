//! The builtins group's many-words jobs (../builtins-NOTES.md, "Many words"): find every place any of K words occurs
//! in a log, one way per kernel named `bwords-K.rust.WAY`. Every way gets the same log and words and prints
//! `"{K}:{matches}:{sum of match start offsets}"`. The words are 8-digit ids hidden inside longer digit runs (session
//! tokens), and no two matches overlap in these logs.

use std::collections::HashSet;
use std::fmt::Write;

use aho_corasick::AhoCorasick;
use regex::Regex;

use crate::inputs::XorShift32;
use crate::work::Outcome;
use crate::{Args, measure};

const POOL: u32 = 10_000;
const METHODS: [&str; 4] = ["GET", "POST", "PUT", "DELETE"];
const STATUSES: [u32; 12] = [200, 201, 204, 301, 304, 400, 401, 403, 404, 500, 502, 503];

type Search = fn(&str, &[String]) -> Result<(u64, u64), String>;

pub fn run(args: &Args) -> Result<Option<Outcome>, String> {
    let Some((job, way)) = args.kernel.split_once(".rust.") else {
        return Ok(None);
    };
    let Some(k) = job
        .strip_prefix("bwords-")
        .and_then(|k| k.parse::<usize>().ok())
    else {
        return Ok(None);
    };
    let search: Search = match way {
        "str.match_indices" => find_loop,
        "regex" => regex_alternation,
        "aho-corasick" => aho_corasick,
        _ => return Ok(None),
    };
    let (text, users) = words_log(args.size);
    let words = first_distinct(&users, k);
    let mut failed = None;
    let (found, times) = measure(
        args.repeats,
        || (),
        |()| match search(&text, &words) {
            Ok(r) => r,
            Err(e) => {
                failed = Some(e);
                (0, 0)
            }
        },
    );
    if let Some(e) = failed {
        return Err(e);
    }
    Ok(Some((format!("{k}:{}:{}", found.0, found.1), times, None)))
}

/// 0 to 6 random digits: the length first, then each digit.
fn digits(rng: &mut XorShift32) -> String {
    let n = rng.next_u32() % 7;
    (0..n)
        .map(|_| char::from(b'0' + (rng.next_u32() % 10) as u8))
        .collect()
}

fn words_log(lines: usize) -> (String, Vec<String>) {
    let mut rng = XorShift32::new();
    let pool: Vec<String> = (0..POOL)
        .map(|_| (10_000_000 + rng.next_u32() % 90_000_000).to_string())
        .collect();
    let mut text = String::new();
    let mut users = Vec::with_capacity(lines);
    for _ in 0..lines {
        let method = METHODS[(rng.next_u32() % 4) as usize];
        let version = 1 + rng.next_u32() % 3;
        let user = &pool[(rng.next_u32() % POOL) as usize];
        users.push(user.clone());
        let before = digits(&mut rng);
        let after = digits(&mut rng);
        let status = STATUSES[(rng.next_u32() % 12) as usize];
        let ms = rng.next_u32() % 1000;
        writeln!(
            text,
            "{method} /api/v{version}/cart?sid={before}{user}{after} {status} {ms}ms"
        )
        .unwrap();
    }
    (text, users)
}

/// The words: the ids of the log's lines in line order, repeats skipped, the first k.
fn first_distinct(users: &[String], k: usize) -> Vec<String> {
    let mut seen = HashSet::new();
    let mut words = Vec::with_capacity(k);
    for w in users {
        if words.len() == k {
            break;
        }
        if seen.insert(w.as_str()) {
            words.push(w.clone());
        }
    }
    words
}

fn find_loop(text: &str, words: &[String]) -> Result<(u64, u64), String> {
    let (mut matches, mut offsets) = (0, 0);
    for w in words {
        for (i, _) in text.match_indices(w.as_str()) {
            matches += 1;
            offsets += i as u64;
        }
    }
    Ok((matches, offsets))
}

fn regex_alternation(text: &str, words: &[String]) -> Result<(u64, u64), String> {
    let pattern = Regex::new(&words.join("|")).map_err(|e| e.to_string())?;
    let (mut matches, mut offsets) = (0, 0);
    for m in pattern.find_iter(text) {
        matches += 1;
        offsets += m.start() as u64;
    }
    Ok((matches, offsets))
}

fn aho_corasick(text: &str, words: &[String]) -> Result<(u64, u64), String> {
    let automaton = AhoCorasick::new(words).map_err(|e| e.to_string())?;
    let (mut matches, mut offsets) = (0, 0);
    for m in automaton.find_iter(text) {
        matches += 1;
        offsets += m.start() as u64;
    }
    Ok((matches, offsets))
}
