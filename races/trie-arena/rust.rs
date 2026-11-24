// Race: trie-arena (rust)
// Extracted from suite/rust/src/arena.rs — build and run via suite/run.py

use std::fmt::Write;
use crate::algo::words;
use crate::bg::MIN_DEPTH;
use crate::inputs::{fnv1a, h31, hashmap_input};
use crate::work::Outcome;
use crate::{Args, measure};

/// Every arena starts with room for 16 nodes, as in C#, Java and JS, then `Vec::push` doubles it.
const FIRST_CAPACITY: usize = 16;

/// Node 0 is the root and never anyone's child, so a child link of 0 means no child.
#[derive(Default)]
struct TrieNode {
    count: u32,
    children: [u32; 26],
}

fn trie_arena(inserted: &[Vec<u8>], prefixes: &[Vec<u8>]) -> String {
    let mut nodes = Vec::with_capacity(FIRST_CAPACITY);
    nodes.push(TrieNode::default());
    for w in inserted {
        let mut node = 0;
        for &ch in w {
            let i = usize::from(ch - b'a');
            if nodes[node].children[i] == 0 {
                nodes[node].children[i] = nodes.len() as u32;
                nodes.push(TrieNode::default());
            }
            node = nodes[node].children[i] as usize;
            nodes[node].count += 1;
        }
    }
    let mut total = 0u64;
    for p in prefixes {
        // A prefix has at least one letter, so node 0 after the walk means it is absent.
        let mut node = 0;
        for &ch in p {
            node = nodes[node].children[usize::from(ch - b'a')] as usize;
            if node == 0 {
                break;
            }
        }
        if node != 0 {
            total += u64::from(nodes[node].count);
        }
    }
    format!("{}:{total}", nodes.len() - 1)
}
