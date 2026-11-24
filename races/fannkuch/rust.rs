// Race: fannkuch (rust)
// Extracted from suite/rust/src/bg.rs — build and run via suite/run.py

use std::f64::consts::PI;
use std::fmt::Write;

pub fn fannkuch(n: usize) -> String {
    let mut perm1: Vec<usize> = (0..n).collect();
    let mut count = vec![0usize; n];
    let mut max_flips = 0;
    let mut checksum: i64 = 0;
    let mut perm_count = 0u64;
    let mut perm = vec![0usize; n];
    let mut r = n;
    loop {
        while r != 1 {
            count[r - 1] = r;
            r -= 1;
        }
        perm.copy_from_slice(&perm1);
        let mut flips = 0;
        let mut k = perm[0];
        while k != 0 {
            perm[..=k].reverse();
            flips += 1;
            k = perm[0];
        }
        max_flips = max_flips.max(flips);
        checksum += if perm_count.is_multiple_of(2) {
            flips
        } else {
            -flips
        };
        loop {
            if r == n {
                return format!("{checksum}:{max_flips}");
            }
            perm1[..=r].rotate_left(1);
            count[r] -= 1;
            if count[r] > 0 {
                break;
            }
            r += 1;
        }
        perm_count += 1;
    }
}
