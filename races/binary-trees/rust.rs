// Race: binary-trees (rust)
// Extracted from suite/rust/src/bg.rs — build and run via suite/run.py

use std::f64::consts::PI;
use std::fmt::Write;

pub struct Node {
    left: Option<Box<Node>>,
    right: Option<Box<Node>>,
}

pub fn make_tree(depth: u32) -> Box<Node> {
    if depth == 0 {
        return Box::new(Node {
            left: None,
            right: None,
        });
    }
    Box::new(Node {
        left: Some(make_tree(depth - 1)),
        right: Some(make_tree(depth - 1)),
    })
}

pub fn check_tree(node: &Node) -> u64 {
    match (&node.left, &node.right) {
        (Some(left), Some(right)) => 1 + check_tree(left) + check_tree(right),
        _ => 1,
    }
}

pub fn depth_checks(depth: u32, iterations: u64) -> u64 {
    (0..iterations).map(|_| check_tree(&make_tree(depth))).sum()
}

pub fn binary_trees(n: u32) -> String {
    binary_trees_lines(n, depth_checks)
}
