//! The -arena kernels (../SPEC.md section 12, "Arena rows"): bst, trie and binary-trees with every node in one
//! growable `Vec` and 32-bit index links, the same algorithms and checksums as the `Box` versions.

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

pub fn run(args: &Args) -> Result<Option<Outcome>, String> {
    let (n, repeats) = (args.size, args.repeats);
    let (checksum, times) = match args.kernel.as_str() {
        "bst-arena" => {
            let (inserts, lookups) = hashmap_input(n);
            measure(repeats, || (), |()| bst_arena(&inserts, &lookups))
        }
        "trie-arena" => {
            let (inserted, prefixes) = words(n);
            measure(repeats, || (), |()| trie_arena(&inserted, &prefixes))
        }
        "binary-trees-arena" => {
            let depth =
                u32::try_from(n).map_err(|_| format!("--size {n} is too deep for a tree"))?;
            let (text, times) = measure(repeats, || depth, binary_trees_arena);
            (fnv1a(text.as_bytes()).to_string(), times)
        }
        _ => return Ok(None),
    };
    Ok(Some((checksum, times, None)))
}

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
