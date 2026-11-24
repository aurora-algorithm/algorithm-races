// Race: binary-trees-arena (rust)
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

/// A binary tree's nodes, children pushed before their parent; a leaf's links are `NIL`.
#[derive(Clone, Copy)]
struct TreeNode {
    left: u32,
    right: u32,
}

fn make_tree(nodes: &mut Vec<TreeNode>, depth: u32) -> u32 {
    let mut node = TreeNode {
        left: NIL,
        right: NIL,
    };
    if depth > 0 {
        node.left = make_tree(nodes, depth - 1);
        node.right = make_tree(nodes, depth - 1);
    }
    nodes.push(node);
    (nodes.len() - 1) as u32
}

fn check_tree(nodes: &[TreeNode], index: u32) -> u64 {
    let node = nodes[index as usize];
    if node.left == NIL {
        return 1;
    }
    1 + check_tree(nodes, node.left) + check_tree(nodes, node.right)
}

fn tree_check(depth: u32) -> u64 {
    let mut nodes = Vec::with_capacity(FIRST_CAPACITY);
    let root = make_tree(&mut nodes, depth);
    check_tree(&nodes, root)
}

fn binary_trees_arena(n: u32) -> String {
    let max_depth = (MIN_DEPTH + 2).max(n);
    let stretch = max_depth + 1;
    let mut out = String::new();
    let stretch_check = tree_check(stretch);
    writeln!(
        out,
        "stretch tree of depth {stretch}\t check: {stretch_check}"
    )
    .unwrap();
    let mut long_lived = Vec::with_capacity(FIRST_CAPACITY);
    let long_root = make_tree(&mut long_lived, max_depth);
    for depth in (MIN_DEPTH..=max_depth).step_by(2) {
        let iterations = 1u64 << (max_depth - depth + MIN_DEPTH);
        let check: u64 = (0..iterations).map(|_| tree_check(depth)).sum();
        writeln!(
            out,
            "{iterations}\t trees of depth {depth}\t check: {check}"
        )
        .unwrap();
    }
    let long_check = check_tree(&long_lived, long_root);
    writeln!(
        out,
        "long lived tree of depth {max_depth}\t check: {long_check}"
    )
    .unwrap();
    out
}
