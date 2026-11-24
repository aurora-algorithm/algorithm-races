// Unusual algorithm kernels (kernels/SPEC.md section 13): one clever trick each, the same steps in every
// language. python/puzzle.py is the reference; the string kernels are in puzzle_text.goose, sudoku in sudoku.goose.

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
let BLOOM_HASHES = 7;
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

let SKIP_MAX_LEVEL = 24;
let SKIP_LEVEL_SEED: u32 = 88675123;

// Nodes in flat arrays with index links, as the Rust version keeps them in a Vec: node i's forward link at
// level l is next[base[i] + l]. Node 0 is the head, with all 24 levels. The arrays are the caller's (empty on
// entry), so skiplist_walk can hash level 0 after the timer.
fn skiplist(keys: i64[:], lookups: i64[:], key: i64[>..]&, base: i64[>..]&, next: i64[>..]&, out: u8[>..]&) {
    var r = xorshift(SKIP_LEVEL_SEED);
    key.push(-1);
    base.push(0);
    next.resize(SKIP_MAX_LEVEL, -1);
    var level = 1;
    var update: i64[24] = [0; 24];
    var heights = 0;
    for k in keys {
        var x = 0;
        var i = level - 1;
        while i >= 0 {
            var nxt = next[base[x] + i];
            while nxt >= 0 && key[nxt] < k {
                x = nxt;
                nxt = next[base[x] + i];
            }
            update[i] = x;
            i--;
        }
        let first = next[base[x]];
        if first >= 0 && key[first] == k { continue; }
        var height = 1;
        while height < SKIP_MAX_LEVEL && r.next() & 1 != 0 { height++; }
        for l in level..height { update[l] = 0; }
        level = max(level, height);
        let node = key.len;
        key.push(k);
        base.push(next.len);
        for l in height {
            next.push(next[base[update[l]] + l]);
            next[base[update[l]] + l] = node;
        }
        heights += height;
    }
    var hits = 0;
    for k in lookups {
        var x = 0;
        var i = level - 1;
        while i >= 0 {
            var nxt = next[base[x] + i];
            while nxt >= 0 && key[nxt] < k {
                x = nxt;
                nxt = next[base[x] + i];
            }
            i--;
        }
        let first = next[base[x]];
        if first >= 0 && key[first] == k { hits++; }
    }
    format(out, keys.len, ":", hits, ":", heights, ":");
}

fn skiplist_walk(key: i64[:], base: i64[:], next: i64[:]) -> u32 {
    var h: u32 = 0;
    var x = next[0];
    while x >= 0 {
        h = h31_step(h, key[x]);
        x = next[base[x]];
    }
    return h;
}

let CYCLE_MOD = 67108859;
let CYCLE_C = 12345;

fn step(x: i64) -> i64 { (x * x + CYCLE_C) % CYCLE_MOD }

fn cycle(starts: i64[:], out: u8[>..]&) {
    var total_mu = 0;
    var total_lam = 0;
    var longest = 0;
    for x0 in starts {
        var tortoise = step(x0);
        var hare = step(tortoise);
        while tortoise != hare {
            tortoise = step(tortoise);
            hare = step(step(hare));
        }
        var mu = 0;
        tortoise = x0;
        while tortoise != hare {
            tortoise = step(tortoise);
            hare = step(hare);
            mu++;
        }
        var lam = 1;
        hare = step(tortoise);
        while tortoise != hare {
            hare = step(hare);
            lam++;
        }
        total_mu += mu;
        total_lam += lam;
        longest = max(longest, mu + lam);
    }
    format(out, starts.len, ":", total_mu, ":", total_lam, ":", longest);
}

let LIFE_SIDE = 512;

fn life_grid() -> u8[>..] {
    var r = xorshift();
    var g: u8[>..] = [];
    for i in LIFE_SIDE * LIFE_SIDE { g.push(if r.next() % 4 == 0 { 1 } else { 0 }); }
    return g;
}

// Runs the generations in the caller's two-grid buffer `cells` (empty on entry) and returns where the last one
// starts, so the alive count and fnv1a run after the timer.
fn life(grid: u8[:], gens: i64, cells: u8[>..]&) -> i64 {
    let w = LIFE_SIDE;
    cells.append(grid);
    cells.resize(2 * grid.len, 0);
    var cur: u8[:] = cells[..grid.len];
    var nxt: u8[:] = cells[grid.len..];
    for g in gens {
        for y in w {
            let up = (y + w - 1) % w * w;
            let row = y * w;
            let down = (y + 1) % w * w;
            for x in w {
                let left = (x + w - 1) % w;
                let right = (x + 1) % w;
                let n = cur[up + left] + cur[up + x] + cur[up + right] + cur[row + left] + cur[row + right]
                    + cur[down + left] + cur[down + x] + cur[down + right];
                nxt[row + x] = if n == 3 || (n == 2 && cur[row + x] != 0) { 1 } else { 0 };
            }
        }
        swap(cur, nxt);
    }
    return if gens % 2 == 0 { 0 } else { grid.len };
}

let WINDOWS = [10, 100, 1000, 10000, 100000];

fn window_max(values: i64[:], out: u8[>..]&) {
    let n = values.len;
    var dq: i64[>..] = [];
    dq.resize(n, 0);
    format(out, n);
    for w in WINDOWS {
        var head = 0;
        var tail = 0;
        var h: u32 = 0;
        for v, i in values {
            while tail > head && values[dq[tail - 1]] <= v { tail--; }
            dq[tail] = i;
            tail++;
            if dq[head] <= i - w { head++; }
            if i >= w - 1 { h = h31_step(h, values[dq[head]]); }
        }
        format(out, ":", h);
    }
}

fn rain_water(heights: i64[:], out: u8[>..]&) {
    let n = heights.len;
    let side = isqrt(n);
    var seen: bool[>..] = [];
    seen.resize(n, false);
    var heap: i64[>..<] = [];
    for idx in n {
        let r = idx / side;
        let c = idx % side;
        if r == 0 || r == side - 1 || c == 0 || c == side - 1 {
            seen[idx] = true;
            heap.push(heights[idx] * n + idx);
        }
    }
    heap.make_heap();
    var water = 0;
    var wet = 0;
    while heap.len > 0 {
        let top = heap.heap_pop();
        let level = top / n;
        let idx = top % n;
        let r = idx / side;
        let c = idx % side;
        for d in 4 {
            let nr = r + [-1, 1, 0, 0][d];
            let nc = c + [0, 0, -1, 1][d];
            if nr < 0 || nr >= side || nc < 0 || nc >= side { continue; }
            let nidx = nr * side + nc;
            if seen[nidx] { continue; }
            seen[nidx] = true;
            var h = heights[nidx];
            if h < level {
                water += level - h;
                wet++;
                h = level;
            }
            heap.heap_push(h * n + nidx);
        }
    }
    format(out, side, ":", water, ":", wet);
}

fn collatz(n: i64, out: u8[>..]&) {
    var cache: i32[>..] = [];
    cache.resize(n, 0);
    cache[1] = 1;
    var best = 1;
    var best_len: i32 = 1;
    var total = 1;
    for start in 2..n {
        var x = start;
        var steps: i32 = 0;
        while x >= start {
            x = if x % 2 == 0 { x / 2 } else { 3 * x + 1 };
            steps++;
        }
        let len = steps + cache[x];
        cache[start] = len;
        total += len;
        if len > best_len {
            best = start;
            best_len = len;
        }
    }
    format(out, n, ":", best, ":", best_len, ":", total);
}
