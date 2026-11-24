//! The string kernels of group `puzzle` (../SPEC.md section 13): palindrome, bwt, aho-corasick, huffman.

use std::cmp::Reverse;
use std::collections::BinaryHeap;

use crate::inputs::XorShift32;

const ONE_MIB: usize = 1 << 20;
const DNA: &[u8; 4] = b"ACGT";

// palindrome: Manacher's mirror trick finds every palindrome in linear time

/// `a`/`b` from the bits of generator values, low bit first, 32 characters per value.
pub fn binary_text(n: usize) -> Vec<u8> {
    let mut rng = XorShift32::new();
    let mut out = vec![0u8; n];
    for chunk in out.chunks_mut(32) {
        let mut v = rng.next_u32();
        for ch in chunk {
            *ch = if v & 1 == 1 { b'b' } else { b'a' };
            v >>= 1;
        }
    }
    out
}

pub fn palindrome(text: &[u8]) -> String {
    let mut t = vec![b'#'; 2 * text.len() + 3];
    let last = t.len() - 1;
    t[0] = b'^';
    t[last] = b'$';
    for (i, &ch) in text.iter().enumerate() {
        t[2 + 2 * i] = ch;
    }
    let mut p = vec![0usize; t.len()];
    let (mut center, mut right) = (0, 0);
    for i in 1..last {
        if i < right {
            p[i] = (right - i).min(p[2 * center - i]);
        }
        while t[i + 1 + p[i]] == t[i - 1 - p[i]] {
            p[i] += 1;
        }
        if i + p[i] > right {
            center = i;
            right = i + p[i];
        }
    }
    let (mut longest, mut best, mut count) = (0, 0, 0);
    for (i, &radius) in p.iter().enumerate() {
        count += radius.div_ceil(2);
        if radius > longest {
            longest = radius;
            best = i;
        }
    }
    format!(
        "{}:{longest}:{}:{count}",
        text.len(),
        (best - 1 - longest) / 2
    )
}

// bwt: suffix array by prefix doubling, then the Burrows-Wheeler transform of a DNA string

/// `ACGT` from 2-bit chunks of generator values, low bits first, 16 characters per value.
fn dna_from(rng: &mut XorShift32, n: usize) -> Vec<u8> {
    let mut out = vec![0u8; n];
    for chunk in out.chunks_mut(16) {
        let mut v = rng.next_u32();
        for ch in chunk {
            *ch = DNA[(v & 3) as usize];
            v >>= 2;
        }
    }
    out
}

pub fn dna_text(n: usize) -> Vec<u8> {
    dna_from(&mut XorShift32::new(), n)
}

/// `(n, rounds, primary, bwt)`; the checksum's fnv1a runs after the timer.
pub fn bwt(dna: &[u8]) -> (usize, usize, usize, Vec<u8>) {
    let text = [dna, b"$"].concat();
    let n = text.len();
    let mut rank: Vec<i32> = text.iter().map(|&b| i32::from(b)).collect();
    let mut sa: Vec<usize> = (0..n).collect();
    let mut second = vec![0i32; n];
    let mut tmp = vec![0i32; n];
    let mut k = 1;
    let mut rounds = 0;
    loop {
        rounds += 1;
        for i in 0..n {
            second[i] = if i + k < n { rank[i + k] } else { -1 };
        }
        sa.sort_by(|&a, &b| (rank[a], second[a]).cmp(&(rank[b], second[b])));
        tmp[sa[0]] = 0;
        for j in 1..n {
            let (a, b) = (sa[j - 1], sa[j]);
            let differs = rank[a] != rank[b] || second[a] != second[b];
            tmp[b] = tmp[a] + i32::from(differs);
        }
        std::mem::swap(&mut rank, &mut tmp);
        if rank[sa[n - 1]] as usize == n - 1 {
            break;
        }
        k *= 2;
    }
    let mut primary = 0;
    let out: Vec<u8> = sa
        .iter()
        .enumerate()
        .map(|(j, &i)| {
            if i == 0 {
                primary = j;
                text[n - 1]
            } else {
                text[i - 1]
            }
        })
        .collect();
    (n, rounds, primary, out)
}

// aho-corasick: a trie with failure links matches thousands of patterns in one pass

const AHO_PATTERNS: usize = 10000;

pub fn aho_input(n: usize) -> (Vec<Vec<u8>>, Vec<u8>) {
    let mut rng = XorShift32::new();
    let patterns = (0..AHO_PATTERNS)
        .map(|_| {
            let len = 6 + rng.next_u32() % 7;
            (0..len)
                .map(|_| DNA[(rng.next_u32() % 4) as usize])
                .collect()
        })
        .collect();
    let block = dna_from(&mut rng, ONE_MIB);
    (patterns, block.repeat(n / ONE_MIB))
}

pub fn aho_corasick(patterns: &[Vec<u8>], text: &[u8]) -> String {
    let mut code = [0usize; 256];
    for (i, &ch) in DNA.iter().enumerate() {
        code[usize::from(ch)] = i;
    }
    let mut goto: Vec<i32> = vec![-1; 4];
    let mut out: Vec<u64> = vec![0];
    for pattern in patterns {
        let mut s = 0;
        for &ch in pattern {
            let edge = s * 4 + code[usize::from(ch)];
            if goto[edge] == -1 {
                goto[edge] = out.len() as i32;
                goto.extend([-1; 4]);
                out.push(0);
            }
            s = goto[edge] as usize;
        }
        out[s] += 1;
    }
    let mut fail = vec![0usize; out.len()];
    let mut queue = Vec::with_capacity(out.len());
    for edge in goto.iter_mut().take(4) {
        if *edge == -1 {
            *edge = 0;
        } else {
            queue.push(*edge as usize);
        }
    }
    let mut head = 0;
    while head < queue.len() {
        let s = queue[head];
        head += 1;
        out[s] += out[fail[s]];
        for c in 0..4 {
            let fallback = goto[fail[s] * 4 + c];
            match goto[s * 4 + c] {
                -1 => goto[s * 4 + c] = fallback,
                t => {
                    fail[t as usize] = fallback as usize;
                    queue.push(t as usize);
                }
            }
        }
    }
    let (mut s, mut matches, mut hits) = (0usize, 0u64, 0u64);
    for &ch in text {
        s = goto[s * 4 + code[usize::from(ch)]] as usize;
        if out[s] > 0 {
            matches += out[s];
            hits += 1;
        }
    }
    format!("{}:{}:{matches}:{hits}", patterns.len(), out.len())
}

// huffman: merge the two rarest symbols until one tree is left

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
