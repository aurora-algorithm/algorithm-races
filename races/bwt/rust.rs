// Race: bwt (rust)
// Extracted from suite/rust/src/puzzle_text.rs — build and run via suite/run.py

use std::cmp::Reverse;
use std::collections::BinaryHeap;
use crate::inputs::XorShift32;

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
