// Race: aho-corasick (rust)
// Extracted from suite/rust/src/puzzle_text.rs — build and run via suite/run.py

use std::cmp::Reverse;
use std::collections::BinaryHeap;
use crate::inputs::XorShift32;

const ONE_MIB: usize = 1 << 20;

const DNA: &[u8; 4] = b"ACGT";

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
