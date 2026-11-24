// Race: huffman (rust)
// Extracted from suite/rust/src/puzzle_text.rs — build and run via suite/run.py

use std::cmp::Reverse;
use std::collections::BinaryHeap;
use crate::inputs::XorShift32;

const ONE_MIB: usize = 1 << 20;

/// A 1 MiB block of letters skewed toward `a` (two generator values per letter), repeated.
pub fn skewed_text(n: usize) -> Vec<u8> {
    let mut rng = XorShift32::new();
    let block: Vec<u8> = (0..ONE_MIB)
        .map(|_| {
            let a = rng.next_u32() % 26;
            let b = rng.next_u32() % 26;
            (97 + a * b / 26) as u8
        })
        .collect();
    block.repeat(n / ONE_MIB)
}

/// Children of the internal nodes 256, 257, ...; ids below 256 are leaves (the byte itself).
struct Tree {
    left: [usize; 512],
    right: [usize; 512],
    root: usize,
}

fn huffman_tree(freq: &[u64; 256]) -> Tree {
    let mut heap: BinaryHeap<Reverse<u64>> = (0..256u64)
        .filter(|&sym| freq[sym as usize] > 0)
        .map(|sym| Reverse(freq[sym as usize] * 1024 + sym))
        .collect();
    let mut tree = Tree {
        left: [0; 512],
        right: [0; 512],
        root: 0,
    };
    let mut next_id = 256;
    while heap.len() > 1 {
        let Reverse(a) = heap.pop().expect("heap has two entries");
        let Reverse(b) = heap.pop().expect("heap has two entries");
        tree.left[next_id] = (a % 1024) as usize;
        tree.right[next_id] = (b % 1024) as usize;
        heap.push(Reverse((a / 1024 + b / 1024) * 1024 + next_id as u64));
        next_id += 1;
    }
    tree.root = heap.peek().map_or(0, |&Reverse(top)| (top % 1024) as usize);
    tree
}

fn huffman_codes(tree: &Tree) -> ([u64; 256], [u32; 256]) {
    let mut codes = [0u64; 256];
    let mut lengths = [0u32; 256];
    let mut stack = vec![(tree.root, 0u64, 0u32)];
    while let Some((node, code, len)) = stack.pop() {
        if node < 256 {
            codes[node] = code;
            lengths[node] = len;
        } else {
            stack.push((tree.left[node], code << 1, len + 1));
            stack.push((tree.right[node], (code << 1) | 1, len + 1));
        }
    }
    (codes, lengths)
}

fn huffman_encode(text: &[u8], codes: &[u64; 256], lengths: &[u32; 256]) -> (u64, Vec<u8>) {
    let mut encoded = Vec::new();
    let (mut acc, mut nbits, mut bits) = (0u64, 0u32, 0u64);
    for &b in text {
        let len = lengths[usize::from(b)];
        acc = (acc << len) | codes[usize::from(b)];
        nbits += len;
        bits += u64::from(len);
        while nbits >= 8 {
            nbits -= 8;
            encoded.push((acc >> nbits) as u8);
        }
        acc &= (1 << nbits) - 1;
    }
    if nbits > 0 {
        encoded.push((acc << (8 - nbits)) as u8);
    }
    (bits, encoded)
}

fn huffman_decode(tree: &Tree, encoded: &[u8], n: usize) -> Vec<u8> {
    let mut decoded = Vec::with_capacity(n);
    let mut node = tree.root;
    'bytes: for &byte in encoded {
        for shift in (0..8).rev() {
            node = if (byte >> shift) & 1 == 1 {
                tree.right[node]
            } else {
                tree.left[node]
            };
            if node < 256 {
                decoded.push(node as u8);
                node = tree.root;
                if decoded.len() == n {
                    break 'bytes;
                }
            }
        }
    }
    decoded
}

/// `(n, total bits, encoded, decoded == text)`; the checksum's fnv1a runs after the timer.
pub fn huffman(text: &[u8]) -> (usize, u64, Vec<u8>, bool) {
    let mut freq = [0u64; 256];
    for &b in text {
        freq[usize::from(b)] += 1;
    }
    let tree = huffman_tree(&freq);
    let (codes, lengths) = huffman_codes(&tree);
    let (bits, encoded) = huffman_encode(text, &codes, &lengths);
    let decoded = huffman_decode(&tree, &encoded, text.len());
    (text.len(), bits, encoded, decoded == text)
}
