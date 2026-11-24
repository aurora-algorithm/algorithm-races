// Race: trie (rust)
// Extracted from suite/rust/src/algo.rs — build and run via suite/run.py

use std::cmp::{Ordering, Reverse};
use std::collections::{BTreeMap, BinaryHeap, HashMap, VecDeque};
use crate::inputs::{XorShift32, h31, hashmap_input, ints};
use crate::work::Outcome;
use crate::{Args, measure};

fn word(rng: &mut XorShift32, min_len: u32, spread: u32) -> Vec<u8> {
    let len = min_len + rng.next_u32() % spread;
    (0..len)
        .map(|_| b'a' + (rng.next_u32() % 26) as u8)
        .collect()
}

pub fn words(n: usize) -> (Vec<Vec<u8>>, Vec<Vec<u8>>) {
    let mut rng = XorShift32::new();
    let inserted = (0..n).map(|_| word(&mut rng, 3, 8)).collect();
    let prefixes = (0..n).map(|_| word(&mut rng, 1, 4)).collect();
    (inserted, prefixes)
}

#[derive(Default)]
struct TrieNode {
    count: u64,
    children: [Option<Box<TrieNode>>; 26],
}

fn trie(inserted: &[Vec<u8>], prefixes: &[Vec<u8>]) -> String {
    let mut root = TrieNode::default();
    let mut nodes = 0;
    for w in inserted {
        let mut node = &mut root;
        for &ch in w {
            node = node.children[usize::from(ch - b'a')].get_or_insert_with(|| {
                nodes += 1;
                Box::default()
            });
            node.count += 1;
        }
    }
    let mut total = 0;
    for p in prefixes {
        let mut node = Some(&root);
        for &ch in p {
            node = node.and_then(|n| n.children[usize::from(ch - b'a')].as_deref());
        }
        total += node.map_or(0, |n| n.count);
    }
    format!("{nodes}:{total}")
}
