//! The autocomplete kernels (../SPEC.md section 15): the first 10 words that start with a typed prefix, from a
//! sorted list with binary search, a 26-slot trie of boxed nodes, or the same trie in one arena `Vec`.

use std::hint::black_box;
use std::path::Path;
use std::time::Instant;

use serde_json::json;

use crate::inputs::XorShift32;
use crate::work::Outcome;
use crate::{Args, elapsed_ms, measure};

const WORDS_FILE: &str = "words-web2.txt";
const TOP_K: usize = 10;
const NO_WORD: i32 = -1;
/// The arena starts with room for 16 nodes, as the other -arena kernels do, then `Vec::push` doubles it.
const FIRST_CAPACITY: usize = 16;

pub fn run(args: &Args) -> Result<Option<Outcome>, String> {
    let (n, repeats) = (args.size, args.repeats);
    let outcome = match args.kernel.as_str() {
        "ac-sorted-build" => {
            let words = build_input(n, &args.data)?;
            let (sorted, times) = time_builds(repeats, || words.clone(), sorted_build);
            (build_check(&sorted), times, None)
        }
        "ac-trie-build" => {
            let words = build_input(n, &args.data)?;
            let ((root, nodes), times) = time_builds(repeats, || &words, |w| trie_build(w));
            let mut out = Vec::new();
            collect(&root, &words, &mut out, words.len() + 1);
            (build_check(&out), times, Some(json!({ "nodes": nodes })))
        }
        "ac-trie-arena-build" => {
            let words = build_input(n, &args.data)?;
            let (arena, times) = time_builds(repeats, || &words, |w| arena_build(w));
            let mut out = Vec::new();
            arena_collect(&arena, 0, &words, &mut out, words.len() + 1);
            (
                build_check(&out),
                times,
                Some(json!({ "nodes": arena.len() })),
            )
        }
        "ac-sorted" => {
            let (words, queries) = query_input(n, &args.data)?;
            let sorted = sorted_build(words);
            let complete = |q: &str, out: &mut Vec<_>| sorted_complete(&sorted, q, out);
            query_outcome(
                &queries,
                measure(repeats, || (), |()| run_queries(&complete, &queries)),
            )
        }
        "ac-trie" => {
            let (words, queries) = query_input(n, &args.data)?;
            let (root, _) = trie_build(&words);
            let complete = |q: &str, out: &mut Vec<_>| trie_complete(&root, &words, q, out);
            query_outcome(
                &queries,
                measure(repeats, || (), |()| run_queries(&complete, &queries)),
            )
        }
        "ac-trie-arena" => {
            let (words, queries) = query_input(n, &args.data)?;
            let arena = arena_build(&words);
            let complete = |q: &str, out: &mut Vec<_>| arena_complete(&arena, &words, q, out);
            query_outcome(
                &queries,
                measure(repeats, || (), |()| run_queries(&complete, &queries)),
            )
        }
        _ => return Ok(None),
    };
    Ok(Some(outcome))
}

/// Like `measure`, but drops the previous repeat's structure before the next one starts, and keeps the last one
/// alive past its timer, so no freeing happens inside the timed region.
fn time_builds<I, R>(
    repeats: usize,
    mut fresh: impl FnMut() -> I,
    mut build: impl FnMut(I) -> R,
) -> (R, Vec<f64>) {
    let mut times = Vec::with_capacity(repeats);
    let mut last = None;
    for _ in 0..repeats {
        drop(last.take());
        let input = fresh();
        let start = Instant::now();
        let result = black_box(build(black_box(input)));
        times.push(elapsed_ms(start));
        last = Some(result);
    }
    (last.expect("repeats is at least 1"), times)
}

fn shuffled_words(data: &Path) -> Result<(Vec<String>, XorShift32), String> {
    let path = data.join(WORDS_FILE);
    let text = std::fs::read_to_string(&path)
        .map_err(|e| format!("cannot read {}: {e}", path.display()))?;
    let mut words: Vec<String> = text
        .lines()
        .map(|line| line.trim_end_matches('\r'))
        .filter(|line| !line.is_empty())
        .map(String::from)
        .collect();
    let mut rng = XorShift32::new();
    for i in (1..words.len()).rev() {
        let j = rng.next_u32() as usize % (i + 1);
        words.swap(i, j);
    }
    Ok((words, rng))
}

fn build_input(n: usize, data: &Path) -> Result<Vec<String>, String> {
    let (mut words, _) = shuffled_words(data)?;
    words.truncate(n);
    Ok(words)
}

fn query_input(n: usize, data: &Path) -> Result<(Vec<String>, Vec<String>), String> {
    let (words, mut rng) = shuffled_words(data)?;
    let queries = (0..n)
        .map(|_| {
            let w = &words[rng.next_u32() as usize % words.len()];
            let len = 2 + rng.next_u32() as usize % 4;
            w[..len.min(w.len())].to_string()
        })
        .collect();
    Ok((words, queries))
}

fn h31_step(h: u32, v: u32) -> u32 {
    h.wrapping_mul(31).wrapping_add(v)
}

fn fold(h: u32, w: &str) -> u32 {
    let last = w.bytes().last().expect("words are never empty");
    h31_step(h31_step(h, w.len() as u32), u32::from(last))
}

fn build_check<S: AsRef<str>>(in_order: &[S]) -> String {
    let h = in_order.iter().fold(0, |h, w| fold(h, w.as_ref()));
    format!("{}:{h}", in_order.len())
}

fn sorted_build(mut words: Vec<String>) -> Vec<String> {
    words.sort();
    words
}

fn sorted_complete<'a>(a: &'a [String], prefix: &str, out: &mut Vec<&'a str>) {
    let start = a.partition_point(|w| w.as_str() < prefix);
    let matches = a[start..]
        .iter()
        .take(TOP_K)
        .take_while(|w| w.starts_with(prefix));
    out.extend(matches.map(String::as_str));
}

struct TrieNode {
    children: [Option<Box<TrieNode>>; 26],
    word: i32,
}

impl TrieNode {
    fn new() -> Self {
        Self {
            children: Default::default(),
            word: NO_WORD,
        }
    }
}

fn letter(ch: u8) -> usize {
    usize::from(ch - b'a')
}

/// Returns the root and the node count, root included.
fn trie_build(words: &[String]) -> (TrieNode, usize) {
    let mut root = TrieNode::new();
    let mut nodes = 1;
    for (idx, w) in words.iter().enumerate() {
        let mut node = &mut root;
        for ch in w.bytes() {
            node = node.children[letter(ch)].get_or_insert_with(|| {
                nodes += 1;
                Box::new(TrieNode::new())
            });
        }
        node.word = idx as i32;
    }
    (root, nodes)
}

fn collect<'a>(node: &TrieNode, words: &'a [String], out: &mut Vec<&'a str>, limit: usize) {
    if node.word != NO_WORD {
        out.push(&words[node.word as usize]);
    }
    for child in &node.children {
        if out.len() >= limit {
            return;
        }
        if let Some(child) = child {
            collect(child, words, out, limit);
        }
    }
}

fn trie_complete<'a>(root: &TrieNode, words: &'a [String], prefix: &str, out: &mut Vec<&'a str>) {
    let mut node = root;
    for ch in prefix.bytes() {
        match &node.children[letter(ch)] {
            Some(child) => node = child,
            None => return,
        }
    }
    collect(node, words, out, TOP_K);
}

/// Node 0 is the root and never anyone's child, so a child link of 0 means no child.
#[derive(Clone, Copy)]
struct ArenaNode {
    children: [u32; 26],
    word: i32,
}

const EMPTY_NODE: ArenaNode = ArenaNode {
    children: [0; 26],
    word: NO_WORD,
};

fn arena_build(words: &[String]) -> Vec<ArenaNode> {
    let mut nodes = Vec::with_capacity(FIRST_CAPACITY);
    nodes.push(EMPTY_NODE);
    for (idx, w) in words.iter().enumerate() {
        let mut node = 0;
        for ch in w.bytes() {
            let i = letter(ch);
            if nodes[node].children[i] == 0 {
                nodes[node].children[i] = nodes.len() as u32;
                nodes.push(EMPTY_NODE);
            }
            node = nodes[node].children[i] as usize;
        }
        nodes[node].word = idx as i32;
    }
    nodes
}

fn arena_collect<'a>(
    nodes: &[ArenaNode],
    node: usize,
    words: &'a [String],
    out: &mut Vec<&'a str>,
    limit: usize,
) {
    if nodes[node].word != NO_WORD {
        out.push(&words[nodes[node].word as usize]);
    }
    for &child in &nodes[node].children {
        if out.len() >= limit {
            return;
        }
        if child != 0 {
            arena_collect(nodes, child as usize, words, out, limit);
        }
    }
}

fn arena_complete<'a>(
    nodes: &[ArenaNode],
    words: &'a [String],
    prefix: &str,
    out: &mut Vec<&'a str>,
) {
    let mut node = 0;
    for ch in prefix.bytes() {
        node = nodes[node].children[letter(ch)] as usize;
        if node == 0 {
            return;
        }
    }
    arena_collect(nodes, node, words, out, TOP_K);
}

/// Returns the number of results and the fold over every result of every query, in order.
fn run_queries<'a>(
    complete: &impl Fn(&str, &mut Vec<&'a str>),
    queries: &[String],
) -> (usize, u32) {
    let mut results = 0;
    let mut h = 0;
    let mut out = Vec::with_capacity(TOP_K);
    for q in queries {
        out.clear();
        complete(q, &mut out);
        results += out.len();
        h = out.iter().fold(h, |h, w| fold(h, w));
    }
    (results, h)
}

fn query_outcome(queries: &[String], ((results, h), times): ((usize, u32), Vec<f64>)) -> Outcome {
    (format!("{}:{results}:{h}", queries.len()), times, None)
}
