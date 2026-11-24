// Race: bloom (goose)
// Extracted from suite/goose/puzzle.goose — build and run via suite/run.py

import std;
import math;
import gen;

// MurmurHash3's 32-bit finalizer: the one hash every language uses for the probabilistic kernels.
fn fmix32(h0: u32) -> u32 {
    var h = h0;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

let BLOOM_BITS_PER_KEY = 10;

let GOLDEN: u32 = 0x9e3779b9;

fn bloom(keys: i64[:], lookups: i64[:], out: u8[>..]&) {
    let m: u32 = (BLOOM_BITS_PER_KEY * keys.len) as u32;
    var bits: u8[>..] = [];
    bits.resize((m + 7) / 8, 0);
    for key in keys {
        let h1 = fmix32(key as! u32);
        let h2 = fmix32(key as! u32 ^ GOLDEN) | 1;
        for i: u32 in 0..BLOOM_HASHES {
            let b = (h1 + i * h2) % m;
            bits[b >> 3] |= (1 << (b & 7)) as! u8;
        }
    }
    var false_positives = 0;
    for key in lookups {
        let h1 = fmix32(key as! u32);
        let h2 = fmix32(key as! u32 ^ GOLDEN) | 1;
        var all = true;
        for i: u32 in 0..BLOOM_HASHES {
            let b = (h1 + i * h2) % m;
            if bits[b >> 3] & (1 << (b & 7)) as! u8 == 0 {
                all = false;
                break;
            }
        }
        if all { false_positives++; }
    }
    var set_bits = 0;
    for byte in bits { set_bits += popcount(byte); }
    format(out, keys.len, ":", set_bits, ":", false_positives);
}
