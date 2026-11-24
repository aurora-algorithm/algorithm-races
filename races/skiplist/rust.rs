// Race: skiplist (rust)
// Extracted from suite/rust/src/puzzle.rs — build and run via suite/run.py

use std::cmp::Reverse;
use std::collections::BinaryHeap;
use std::iter;
use num_bigint::BigInt;
use crate::inputs::{XorShift32, fnv1a, h31, hashmap_input, ints};
use crate::puzzle_text::{
    aho_corasick, aho_input, binary_text, bwt, dna_text, huffman, palindrome, skewed_text,
};
use crate::work::Outcome;
use crate::{Args, measure};

const SKIP_MAX_LEVEL: usize = 24;

const SKIP_LEVEL_SEED: u32 = 88675123;

const HEAD: usize = 0;

struct SkipNode {
    key: i64,
    next: Vec<Option<usize>>,
}

/// Nodes live in one Vec and link by index (safe Rust has no shared mutable pointers); node 0 is the head.
struct SkipList {
    nodes: Vec<SkipNode>,
    level: usize,
    keys: usize,
    hits: usize,
    heights: usize,
}

impl SkipList {
    fn build(keys: &[i64], lookups: &[i64]) -> Self {
        let head = SkipNode {
            key: -1,
            next: vec![None; SKIP_MAX_LEVEL],
        };
        let mut list = SkipList {
            nodes: vec![head],
            level: 1,
            keys: keys.len(),
            hits: 0,
            heights: 0,
        };
        let mut rng = XorShift32::with_seed(SKIP_LEVEL_SEED);
        for &key in keys {
            list.insert(key, &mut rng);
        }
        list.hits = lookups.iter().filter(|&&key| list.contains(key)).count();
        list
    }

    /// Walks from the top level down to the last node with a smaller key, noting where it dropped a level.
    fn find(&self, key: i64, update: &mut [usize; SKIP_MAX_LEVEL]) -> Option<usize> {
        let mut x = HEAD;
        for i in (0..self.level).rev() {
            while let Some(next) = self.nodes[x].next[i] {
                if self.nodes[next].key >= key {
                    break;
                }
                x = next;
            }
            update[i] = x;
        }
        self.nodes[x].next[0].filter(|&next| self.nodes[next].key == key)
    }

    fn contains(&self, key: i64) -> bool {
        self.find(key, &mut [HEAD; SKIP_MAX_LEVEL]).is_some()
    }

    fn insert(&mut self, key: i64, rng: &mut XorShift32) {
        let mut update = [HEAD; SKIP_MAX_LEVEL];
        if self.find(key, &mut update).is_some() {
            return;
        }
        let mut height = 1;
        while height < SKIP_MAX_LEVEL && rng.next_u32() & 1 == 1 {
            height += 1;
        }
        self.level = self.level.max(height);
        let id = self.nodes.len();
        let next = (0..height).map(|i| self.nodes[update[i]].next[i]).collect();
        self.nodes.push(SkipNode { key, next });
        for (i, &prev) in update.iter().enumerate().take(height) {
            self.nodes[prev].next[i] = Some(id);
        }
        self.heights += height;
    }

    fn checksum(&self) -> String {
        let ordered = iter::successors(self.nodes[HEAD].next[0], |&x| self.nodes[x].next[0])
            .map(|x| self.nodes[x].key);
        format!(
            "{}:{}:{}:{}",
            self.keys,
            self.hits,
            self.heights,
            h31(ordered)
        )
    }
}
