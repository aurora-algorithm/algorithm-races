// Race: bst (rust)
// Extracted from suite/rust/src/algo.rs — build and run via suite/run.py

use std::cmp::{Ordering, Reverse};
use std::collections::{BTreeMap, BinaryHeap, HashMap, VecDeque};
use crate::inputs::{XorShift32, h31, hashmap_input, ints};
use crate::work::Outcome;
use crate::{Args, measure};

struct Node {
    key: i64,
    count: u32,
    left: Option<Box<Node>>,
    right: Option<Box<Node>>,
}

struct Bst {
    root: Option<Box<Node>>,
    size: usize,
    hits: usize,
}

impl Bst {
    fn build(inserts: &[i64], lookups: &[i64]) -> Self {
        let mut tree = Bst {
            root: None,
            size: 0,
            hits: 0,
        };
        for &k in inserts {
            tree.insert(k);
        }
        tree.hits = lookups.iter().filter(|&&k| tree.contains(k)).count();
        tree
    }

    fn insert(&mut self, key: i64) {
        let mut slot = &mut self.root;
        while let Some(node) = slot {
            slot = match key.cmp(&node.key) {
                Ordering::Less => &mut node.left,
                Ordering::Greater => &mut node.right,
                Ordering::Equal => {
                    node.count += 1;
                    return;
                }
            };
        }
        *slot = Some(Box::new(Node {
            key,
            count: 1,
            left: None,
            right: None,
        }));
        self.size += 1;
    }

    fn contains(&self, key: i64) -> bool {
        let mut node = self.root.as_deref();
        while let Some(n) = node {
            node = match key.cmp(&n.key) {
                Ordering::Less => n.left.as_deref(),
                Ordering::Greater => n.right.as_deref(),
                Ordering::Equal => return true,
            };
        }
        false
    }

    fn checksum(&self) -> String {
        let mut keys = Vec::with_capacity(self.size);
        let mut stack = Vec::new();
        let mut node = self.root.as_deref();
        while node.is_some() || !stack.is_empty() {
            while let Some(n) = node {
                stack.push(n);
                node = n.left.as_deref();
            }
            let n = stack
                .pop()
                .expect("the loop condition keeps the stack non-empty");
            keys.push(n.key);
            node = n.right.as_deref();
        }
        format!("{}:{}:{}", self.size, self.hits, h31(keys))
    }
}
