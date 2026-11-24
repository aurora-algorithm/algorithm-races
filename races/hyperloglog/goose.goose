// Race: hyperloglog (goose)
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

let HLL_P = 14;

let HLL_M = 1 << HLL_P;

fn hyperloglog(n: i64, out: u8[>..]&) {
    var r = xorshift();
    let universe: u32 = max(n / 4, 1) as u32;
    var registers: u8[>..] = [];
    registers.resize(HLL_M, 0);
    for i in n {
        let h = fmix32(r.next() % universe);
        let w = h << HLL_P;
        let rank = if w == 0 { 32 - HLL_P + 1 } else { clz(w) - 32 + 1 };
        let idx = h >> (32 - HLL_P);
        if rank > registers[idx] { registers[idx] = rank as! u8; }
    }
    var z = 0.0;
    var zeros = 0;
    for reg in registers {
        z += pow(2.0, -(reg as f64));
        if reg == 0 { zeros++; }
    }
    let m = HLL_M as f64;
    var estimate = 0.7213 / (1.0 + 1.079 / m) * m * m / z;
    if estimate <= 2.5 * m && zeros > 0 { estimate = m * log(m / zeros); }
    var h: u32 = 0;
    for reg in registers { h = h31_step(h, reg); }
    format(out, n, ":", floor(estimate + 0.5) as i64, ":", h);
}
