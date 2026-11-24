// Race: bst-arena (rust)
// Extracted from suite/rust/src/arena.rs — build and run via suite/run.py

use std::fmt::Write;
use crate::algo::words;
use crate::bg::MIN_DEPTH;
use crate::inputs::{fnv1a, h31, hashmap_input};
use crate::work::Outcome;
use crate::{Args, measure};

/// The "no child" link of the BST and the binary trees.
const NIL: u32 = u32::MAX;

/// Every arena starts with room for 16 nodes, as in C#, Java and JS, then `Vec::push` doubles it.
const FIRST_CAPACITY: usize = 16;

struct BstNode {
    key: i64,
    count: u32,
    left: u32,
    right: u32,
}

fn bst_arena(inserts: &[i64], lookups: &[i64]) -> String {
    let mut nodes: Vec<BstNode> = Vec::with_capacity(FIRST_CAPACITY);
    for &k in inserts {
        let fresh = BstNode {
            key: k,
            count: 1,
            left: NIL,
            right: NIL,
        };
        let next_index = nodes.len() as u32;
        if nodes.is_empty() {
            nodes.push(fresh);
            continue;
        }
        let mut cur = 0;
        loop {
            let node = &mut nodes[cur];
            if k == node.key {
                node.count += 1;
                break;
            }
            let link = if k < node.key {
                &mut node.left
            } else {
                &mut node.right
            };
            if *link == NIL {
                *link = next_index;
                nodes.push(fresh);
                break;
            }
            cur = *link as usize;
        }
    }
    let mut hits = 0;
    for &k in lookups {
        let mut cur = if nodes.is_empty() { NIL } else { 0 };
        while cur != NIL && nodes[cur as usize].key != k {
            let node = &nodes[cur as usize];
            cur = if k < node.key { node.left } else { node.right };
        }
        hits += usize::from(cur != NIL);
    }
    format!("{}:{hits}:{}", nodes.len(), h31(in_order(&nodes)))
}

fn in_order(nodes: &[BstNode]) -> Vec<i64> {
    let mut keys = Vec::with_capacity(nodes.len());
    let mut stack = Vec::new();
    let mut cur = if nodes.is_empty() { NIL } else { 0 };
    while cur != NIL || !stack.is_empty() {
        while cur != NIL {
            stack.push(cur);
            cur = nodes[cur as usize].left;
        }
        let top = stack
            .pop()
            .expect("the loop condition keeps the stack non-empty");
        keys.push(nodes[top as usize].key);
        cur = nodes[top as usize].right;
    }
    keys
}
