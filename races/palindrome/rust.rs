// Race: palindrome (rust)
// Extracted from suite/rust/src/puzzle_text.rs — build and run via suite/run.py

use std::cmp::Reverse;
use std::collections::BinaryHeap;
use crate::inputs::XorShift32;

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
