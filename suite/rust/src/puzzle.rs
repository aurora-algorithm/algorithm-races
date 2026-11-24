//! Unusual algorithm kernels (../SPEC.md section 13, group `puzzle`): one clever trick each.

use std::cmp::Reverse;
use std::collections::BinaryHeap;
use std::iter;

use num_bigint::BigInt;

use crate::inputs::{XorShift32, fnv1a, h31, hashmap_input, ints};
use crate::puzzle_text::{
    aho_corasick, aho_input, binary_text, bwt, dna_text, huffman, palindrome, skewed_text,
};
use crate::work::Outcome;
use crate::{Args, measure};

pub fn run(args: &Args) -> Result<Option<Outcome>, String> {
    let (n, repeats) = (args.size, args.repeats);
    let (checksum, times) = match args.kernel.as_str() {
        "sudoku" => {
            let puzzles = sudoku_puzzles(n);
            measure(repeats, || &puzzles, |p| sudoku(p))
        }
        "bloom" => {
            let values = ints(2 * n);
            let (keys, lookups) = values.split_at(n);
            measure(repeats, || (), |()| bloom(keys, lookups))
        }
        "hyperloglog" => measure(repeats, || n, hyperloglog),
        "skiplist" => {
            let (keys, lookups) = hashmap_input(n);
            let (list, times) = measure(repeats, || (), |()| SkipList::build(&keys, &lookups));
            (list.checksum(), times)
        }
        "cycle" => {
            let starts = cycle_starts(n);
            measure(repeats, || &starts, |s| cycle(s))
        }
        "palindrome" => {
            let text = binary_text(n);
            measure(repeats, || &text, |t| palindrome(t))
        }
        "bwt" => {
            let dna = dna_text(n);
            let ((len, rounds, primary, out), times) = measure(repeats, || &dna, |d| bwt(d));
            (format!("{len}:{rounds}:{primary}:{}", fnv1a(&out)), times)
        }
        "aho-corasick" => {
            let (patterns, text) = aho_input(n);
            measure(repeats, || (), |()| aho_corasick(&patterns, &text))
        }
        "pi-digits" => {
            let (digits, times) = measure(repeats, || n, pi_digits);
            (pi_checksum(&digits), times)
        }
        "life" => {
            let grid = life_input();
            let (cells, times) = measure(repeats, || &grid, |g| life(g, n));
            let alive: usize = cells.iter().map(|&c| usize::from(c)).sum();
            (format!("{n}:{alive}:{}", fnv1a(&cells)), times)
        }
        "huffman" => {
            let text = skewed_text(n);
            let ((len, bits, encoded, ok), times) = measure(repeats, || &text, |t| huffman(t));
            (
                format!("{len}:{bits}:{}:{}", fnv1a(&encoded), u8::from(ok)),
                times,
            )
        }
        "window-max" => {
            let values = ints(n);
            measure(repeats, || &values, |v| window_max(v))
        }
        "rain-water" => {
            let heights = terrain(n);
            measure(repeats, || &heights, |h| rain_water(n, h))
        }
        "collatz" => measure(repeats, || n, collatz),
        _ => return Ok(None),
    };
    Ok(Some((checksum, times, None)))
}

/// MurmurHash3's 32-bit finalizer: the one hash every language uses for the probabilistic kernels.
fn fmix32(mut h: u32) -> u32 {
    h ^= h >> 16;
    h = h.wrapping_mul(0x85eb_ca6b);
    h ^= h >> 13;
    h = h.wrapping_mul(0xc2b2_ae35);
    h ^= h >> 16;
    h
}

fn shuffle<T>(rng: &mut XorShift32, items: &mut [T]) {
    for i in (1..items.len()).rev() {
        let j = rng.next_u32() as usize % (i + 1);
        items.swap(i, j);
    }
}

// sudoku: Knuth's Algorithm X with dancing links

const SUDOKU_CLUES: usize = 26;
const DLX_COLUMNS: usize = 324;

fn sudoku_puzzles(n: usize) -> Vec<[u8; 81]> {
    let mut rng = XorShift32::new();
    (0..n)
        .map(|_| {
            let mut digits: Vec<u8> = (1..=9).collect();
            shuffle(&mut rng, &mut digits);
            let rows = shuffled_lines(&mut rng);
            let cols = shuffled_lines(&mut rng);
            let mut cells: Vec<usize> = (0..81).collect();
            shuffle(&mut rng, &mut cells);
            let mut puzzle = [0u8; 81];
            for &cell in &cells[..SUDOKU_CLUES] {
                let (r, c) = (rows[cell / 9], cols[cell % 9]);
                puzzle[cell] = digits[(r * 3 + r / 3 + c) % 9];
            }
            puzzle
        })
        .collect()
}

/// Rows (or columns) of a valid grid, shuffled within and between their bands.
fn shuffled_lines(rng: &mut XorShift32) -> Vec<usize> {
    let mut bands = [0, 1, 2];
    shuffle(rng, &mut bands);
    let mut lines = Vec::with_capacity(9);
    for band in bands {
        let mut inner = [0, 1, 2];
        shuffle(rng, &mut inner);
        lines.extend(inner.iter().map(|i| band * 3 + i));
    }
    lines
}

/// The exact-cover matrix as flat arrays: node 0 is the root, 1..324 the column headers, then 4 nodes per row.
struct Dlx {
    left: Vec<usize>,
    right: Vec<usize>,
    up: Vec<usize>,
    down: Vec<usize>,
    col: Vec<usize>,
    row: Vec<usize>,
    count: Vec<u32>,
    solutions: u64,
    tries: u64,
    chosen: [usize; 81],
    first: Vec<usize>,
}

impl Dlx {
    fn new() -> Self {
        let size = 1 + DLX_COLUMNS + 729 * 4;
        let mut dlx = Dlx {
            left: vec![0; size],
            right: vec![0; size],
            up: (0..size).collect(),
            down: (0..size).collect(),
            col: (0..size).collect(),
            row: vec![0; size],
            count: vec![0; DLX_COLUMNS + 1],
            solutions: 0,
            tries: 0,
            chosen: [0; 81],
            first: Vec::new(),
        };
        for i in 0..=DLX_COLUMNS {
            dlx.left[i] = (i + DLX_COLUMNS) % (DLX_COLUMNS + 1);
            dlx.right[i] = (i + 1) % (DLX_COLUMNS + 1);
        }
        let mut node = DLX_COLUMNS + 1;
        for r in 0..9 {
            for c in 0..9 {
                let b = (r / 3) * 3 + c / 3;
                for d in 0..9 {
                    let columns = [
                        1 + r * 9 + c,
                        82 + r * 9 + d,
                        163 + c * 9 + d,
                        244 + b * 9 + d,
                    ];
                    dlx.add_row(node, (r * 9 + c) * 9 + d, columns);
                    node += 4;
                }
            }
        }
        dlx
    }

    /// Appends the four nodes `first..first + 4` at the bottom of their columns and links them in a ring.
    fn add_row(&mut self, first: usize, row: usize, columns: [usize; 4]) {
        for (k, column) in columns.into_iter().enumerate() {
            let node = first + k;
            self.col[node] = column;
            self.row[node] = row;
            self.up[node] = self.up[column];
            self.down[node] = column;
            self.down[self.up[column]] = node;
            self.up[column] = node;
            self.count[column] += 1;
            self.left[node] = node - 1;
            self.right[node] = node + 1;
        }
        self.left[first] = first + 3;
        self.right[first + 3] = first;
    }

    fn cover(&mut self, c: usize) {
        self.right[self.left[c]] = self.right[c];
        self.left[self.right[c]] = self.left[c];
        let mut i = self.down[c];
        while i != c {
            let mut j = self.right[i];
            while j != i {
                self.down[self.up[j]] = self.down[j];
                self.up[self.down[j]] = self.up[j];
                self.count[self.col[j]] -= 1;
                j = self.right[j];
            }
            i = self.down[i];
        }
    }

    fn uncover(&mut self, c: usize) {
        let mut i = self.up[c];
        while i != c {
            let mut j = self.left[i];
            while j != i {
                self.count[self.col[j]] += 1;
                self.down[self.up[j]] = j;
                self.up[self.down[j]] = j;
                j = self.left[j];
            }
            i = self.up[i];
        }
        self.right[self.left[c]] = c;
        self.left[self.right[c]] = c;
    }

    fn choose_row(&mut self, node: usize) {
        let mut j = self.right[node];
        while j != node {
            self.cover(self.col[j]);
            j = self.right[j];
        }
    }

    fn unchoose_row(&mut self, node: usize) {
        let mut j = self.left[node];
        while j != node {
            self.uncover(self.col[j]);
            j = self.left[j];
        }
    }

    fn search(&mut self, depth: usize) {
        if self.right[0] == 0 {
            self.solutions += 1;
            if self.solutions == 1 {
                self.first = self.chosen[..depth].to_vec();
            }
            return;
        }
        let mut best = self.right[0];
        let mut c = self.right[best];
        while c != 0 {
            if self.count[c] < self.count[best] {
                best = c;
            }
            c = self.right[c];
        }
        self.cover(best);
        let mut r = self.down[best];
        while r != best {
            self.tries += 1;
            self.chosen[depth] = self.row[r];
            self.choose_row(r);
            self.search(depth + 1);
            self.unchoose_row(r);
            r = self.down[r];
        }
        self.uncover(best);
    }
}

fn sudoku(puzzles: &[[u8; 81]]) -> String {
    let (mut solutions, mut tries, mut h) = (0, 0, 0u32);
    for puzzle in puzzles {
        let mut dlx = Dlx::new();
        let mut depth = 0;
        for (cell, &digit) in puzzle.iter().enumerate() {
            if digit > 0 {
                let row = cell * 9 + usize::from(digit) - 1;
                let node = DLX_COLUMNS + 1 + row * 4;
                dlx.cover(dlx.col[node]);
                dlx.choose_row(node);
                dlx.chosen[depth] = row;
                depth += 1;
            }
        }
        dlx.search(depth);
        solutions += dlx.solutions;
        tries += dlx.tries;
        let mut grid = [0i64; 81];
        for &row in &dlx.first {
            grid[row / 9] = (row % 9 + 1) as i64;
        }
        h = h31(iter::once(i64::from(h)).chain(grid));
    }
    format!("{}:{solutions}:{tries}:{h}", puzzles.len())
}

// bloom: k bit positions per key from two hashes

const BLOOM_BITS_PER_KEY: usize = 10;
const BLOOM_HASHES: u32 = 7;
const GOLDEN: u32 = 0x9e37_79b9;

/// The bit positions of a key: `h1 + i*h2` (Kirsch-Mitzenmacher), wrapping at 2^32, modulo m.
fn bloom_bits(key: i64, m: u32) -> impl Iterator<Item = usize> {
    let h1 = fmix32(key as u32);
    let h2 = fmix32(key as u32 ^ GOLDEN) | 1;
    (0..BLOOM_HASHES).map(move |i| (h1.wrapping_add(i.wrapping_mul(h2)) % m) as usize)
}

fn bloom(keys: &[i64], lookups: &[i64]) -> String {
    let m = BLOOM_BITS_PER_KEY * keys.len();
    let mut bits = vec![0u8; m.div_ceil(8)];
    let m = m as u32;
    for &key in keys {
        for b in bloom_bits(key, m) {
            bits[b >> 3] |= 1 << (b & 7);
        }
    }
    let false_positives = lookups
        .iter()
        .filter(|&&key| bloom_bits(key, m).all(|b| bits[b >> 3] & (1 << (b & 7)) != 0))
        .count();
    let set_bits: u32 = bits.iter().map(|byte| byte.count_ones()).sum();
    format!("{}:{set_bits}:{false_positives}", keys.len())
}

// hyperloglog: the longest run of leading zeros estimates how many distinct items went by

const HLL_P: u32 = 14;
const HLL_M: usize = 1 << HLL_P;

fn hyperloglog(n: usize) -> String {
    let mut rng = XorShift32::new();
    let universe = (n / 4).max(1) as u32;
    let mut registers = vec![0u8; HLL_M];
    for _ in 0..n {
        let h = fmix32(rng.next_u32() % universe);
        let w = h << HLL_P;
        let rank = if w == 0 {
            32 - HLL_P + 1
        } else {
            w.leading_zeros() + 1
        };
        let idx = (h >> (32 - HLL_P)) as usize;
        registers[idx] = registers[idx].max(rank as u8);
    }
    let mut z = 0.0;
    let mut zeros = 0;
    for &r in &registers {
        z += 2f64.powi(-i32::from(r));
        zeros += usize::from(r == 0);
    }
    let m = HLL_M as f64;
    let mut estimate = 0.7213 / (1.0 + 1.079 / m) * m * m / z;
    if estimate <= 2.5 * m && zeros > 0 {
        estimate = m * (m / zeros as f64).ln();
    }
    let registers = registers.iter().map(|&r| i64::from(r));
    format!("{n}:{}:{}", (estimate + 0.5).floor(), h31(registers))
}

// skiplist: towers of random height turn a linked list into a search tree

const SKIP_MAX_LEVEL: usize = 24;
const SKIP_LEVEL_SEED: u32 = 88675123;
const HEAD: usize = 0;

struct SkipNode {
    key: i64,
    next: Vec<Option<usize>>,
}

/// Nodes live in one Vec and link by index (safe Rust has no shared mutable pointers); node 0 is the head.
struct SkipList {
    nodes: Vec<SkipNode>,
    level: usize,
    keys: usize,
    hits: usize,
    heights: usize,
}

impl SkipList {
    fn build(keys: &[i64], lookups: &[i64]) -> Self {
        let head = SkipNode {
            key: -1,
            next: vec![None; SKIP_MAX_LEVEL],
        };
        let mut list = SkipList {
            nodes: vec![head],
            level: 1,
            keys: keys.len(),
            hits: 0,
            heights: 0,
        };
        let mut rng = XorShift32::with_seed(SKIP_LEVEL_SEED);
        for &key in keys {
            list.insert(key, &mut rng);
        }
        list.hits = lookups.iter().filter(|&&key| list.contains(key)).count();
        list
    }

    /// Walks from the top level down to the last node with a smaller key, noting where it dropped a level.
    fn find(&self, key: i64, update: &mut [usize; SKIP_MAX_LEVEL]) -> Option<usize> {
        let mut x = HEAD;
        for i in (0..self.level).rev() {
            while let Some(next) = self.nodes[x].next[i] {
                if self.nodes[next].key >= key {
                    break;
                }
                x = next;
            }
            update[i] = x;
        }
        self.nodes[x].next[0].filter(|&next| self.nodes[next].key == key)
    }

    fn contains(&self, key: i64) -> bool {
        self.find(key, &mut [HEAD; SKIP_MAX_LEVEL]).is_some()
    }

    fn insert(&mut self, key: i64, rng: &mut XorShift32) {
        let mut update = [HEAD; SKIP_MAX_LEVEL];
        if self.find(key, &mut update).is_some() {
            return;
        }
        let mut height = 1;
        while height < SKIP_MAX_LEVEL && rng.next_u32() & 1 == 1 {
            height += 1;
        }
        self.level = self.level.max(height);
        let id = self.nodes.len();
        let next = (0..height).map(|i| self.nodes[update[i]].next[i]).collect();
        self.nodes.push(SkipNode { key, next });
        for (i, &prev) in update.iter().enumerate().take(height) {
            self.nodes[prev].next[i] = Some(id);
        }
        self.heights += height;
    }

    fn checksum(&self) -> String {
        let ordered = iter::successors(self.nodes[HEAD].next[0], |&x| self.nodes[x].next[0])
            .map(|x| self.nodes[x].key);
        format!(
            "{}:{}:{}:{}",
            self.keys,
            self.hits,
            self.heights,
            h31(ordered)
        )
    }
}

// cycle: Floyd's tortoise and hare on x -> x*x + c mod M

const CYCLE_MOD: u64 = 67108859;
const CYCLE_C: u64 = 12345;

fn cycle_starts(n: usize) -> Vec<u64> {
    let mut rng = XorShift32::new();
    (0..n)
        .map(|_| u64::from(rng.next_u32()) % CYCLE_MOD)
        .collect()
}

fn step(x: u64) -> u64 {
    (x * x + CYCLE_C) % CYCLE_MOD
}

fn cycle(starts: &[u64]) -> String {
    let (mut total_mu, mut total_lam, mut longest) = (0u64, 0u64, 0u64);
    for &x0 in starts {
        let mut tortoise = step(x0);
        let mut hare = step(step(x0));
        while tortoise != hare {
            tortoise = step(tortoise);
            hare = step(step(hare));
        }
        let mut mu = 0;
        tortoise = x0;
        while tortoise != hare {
            tortoise = step(tortoise);
            hare = step(hare);
            mu += 1;
        }
        let mut lam = 1;
        hare = step(tortoise);
        while tortoise != hare {
            hare = step(hare);
            lam += 1;
        }
        total_mu += mu;
        total_lam += lam;
        longest = longest.max(mu + lam);
    }
    format!("{}:{total_mu}:{total_lam}:{longest}", starts.len())
}

// pi-digits: the Chudnovsky series summed by binary splitting

const CHUD_A: u64 = 13591409;
const CHUD_B: u64 = 545140134;
const CHUD_C3_24: u64 = 10939058860032000;
const DIGITS_PER_TERM: usize = 14;
const GUARD_DIGITS: u32 = 10;

/// P(a,b), Q(a,b), T(a,b) of the Chudnovsky terms a .. b-1.
fn binary_split(a: u64, b: u64) -> (BigInt, BigInt, BigInt) {
    if b - a == 1 {
        let (p, q) = if a == 0 {
            (BigInt::from(1), BigInt::from(1))
        } else {
            let p = BigInt::from((6 * a - 5) * (2 * a - 1) * (6 * a - 1));
            (p, BigInt::from(a).pow(3) * CHUD_C3_24)
        };
        let t = &p * (CHUD_A + CHUD_B * a);
        return (p, q, if a % 2 == 1 { -t } else { t });
    }
    let m = (a + b) / 2;
    let (p1, q1, t1) = binary_split(a, m);
    let (p2, q2, t2) = binary_split(m, b);
    (&p1 * &p2, q1 * &q2, t1 * q2 + p1 * t2)
}

/// Floor square root of n > 0 by precision doubling (CPython's `math.isqrt`), written out as in the others.
fn isqrt(n: &BigInt) -> BigInt {
    let c = (n.bits() - 1) / 2;
    let mut a = BigInt::from(1);
    let mut d = 0;
    for s in (0..u64::BITS - c.leading_zeros()).rev() {
        let e = d;
        d = c >> s;
        a = (&a << (d - e - 1)) + (n >> (2 * c - e - d + 1)) / &a;
    }
    if &a * &a > *n { a - 1 } else { a }
}

fn pi_digits(n: usize) -> String {
    let terms = (n / DIGITS_PER_TERM + 2) as u64;
    let (_p, q, t) = binary_split(0, terms);
    let one = BigInt::from(10).pow(n as u32 + GUARD_DIGITS);
    let sqrt_c = isqrt(&(BigInt::from(10005) * &one * &one));
    let pi = BigInt::from(426880) * sqrt_c * q / t;
    (pi / BigInt::from(10).pow(GUARD_DIGITS)).to_string()
}

fn pi_checksum(digits: &str) -> String {
    let last = &digits[digits.len().saturating_sub(50)..];
    let sum: u32 = digits.bytes().map(|d| u32::from(d - b'0')).sum();
    format!("{}:{last}:{sum}", digits.len() - 1)
}

// life: Conway's Game of Life on a torus

const LIFE_SIDE: usize = 512;

fn life_input() -> Vec<u8> {
    let mut rng = XorShift32::new();
    (0..LIFE_SIDE * LIFE_SIDE)
        .map(|_| u8::from(rng.next_u32().is_multiple_of(4)))
        .collect()
}

fn life(grid: &[u8], generations: usize) -> Vec<u8> {
    let w = LIFE_SIDE;
    let mut cur = grid.to_vec();
    let mut next = vec![0u8; grid.len()];
    for _ in 0..generations {
        for y in 0..w {
            let up = (y + w - 1) % w * w;
            let row = y * w;
            let down = (y + 1) % w * w;
            for x in 0..w {
                let left = (x + w - 1) % w;
                let right = (x + 1) % w;
                let n = cur[up + left]
                    + cur[up + x]
                    + cur[up + right]
                    + cur[row + left]
                    + cur[row + right]
                    + cur[down + left]
                    + cur[down + x]
                    + cur[down + right];
                next[row + x] = u8::from(n == 3 || (n == 2 && cur[row + x] == 1));
            }
        }
        std::mem::swap(&mut cur, &mut next);
    }
    cur
}

// window-max: a monotonic deque keeps the maximum of every window in O(1) per step

const WINDOWS: [usize; 5] = [10, 100, 1000, 10000, 100000];

fn window_max(values: &[i64]) -> String {
    let n = values.len();
    let mut dq = vec![0usize; n];
    let mut hashes = Vec::with_capacity(WINDOWS.len());
    for w in WINDOWS {
        let (mut head, mut tail) = (0, 0);
        let mut h = 0u32;
        for (i, &v) in values.iter().enumerate() {
            while tail > head && values[dq[tail - 1]] <= v {
                tail -= 1;
            }
            dq[tail] = i;
            tail += 1;
            if dq[head] + w <= i {
                head += 1;
            }
            if i + 1 >= w {
                h = h.wrapping_mul(31).wrapping_add(values[dq[head]] as u32);
            }
        }
        hashes.push(h.to_string());
    }
    format!("{n}:{}", hashes.join(":"))
}

// rain-water: flood a 2D terrain from its walls inward with a min-heap

fn terrain(side: usize) -> Vec<u64> {
    let mut rng = XorShift32::new();
    (0..side * side)
        .map(|_| u64::from(rng.next_u32() % 1000))
        .collect()
}

fn rain_water(side: usize, heights: &[u64]) -> String {
    let n = heights.len();
    let mut seen = vec![false; n];
    let mut heap = BinaryHeap::new();
    for idx in 0..n {
        let (r, c) = (idx / side, idx % side);
        if r == 0 || r == side - 1 || c == 0 || c == side - 1 {
            seen[idx] = true;
            heap.push(Reverse(heights[idx] * n as u64 + idx as u64));
        }
    }
    let (mut water, mut wet) = (0u64, 0u64);
    while let Some(Reverse(key)) = heap.pop() {
        let level = key / n as u64;
        let idx = (key % n as u64) as usize;
        let (r, c) = (idx / side, idx % side);
        let neighbours = [
            (r > 0).then(|| idx - side),
            (r + 1 < side).then(|| idx + side),
            (c > 0).then(|| idx - 1),
            (c + 1 < side).then(|| idx + 1),
        ];
        for next in neighbours.into_iter().flatten() {
            if seen[next] {
                continue;
            }
            seen[next] = true;
            let mut h = heights[next];
            if h < level {
                water += level - h;
                wet += 1;
                h = level;
            }
            heap.push(Reverse(h * n as u64 + next as u64));
        }
    }
    format!("{side}:{water}:{wet}")
}

// collatz: remember every chain length below the start so each walk stops early

fn collatz(n: usize) -> String {
    let mut cache = vec![0u32; n];
    cache[1] = 1;
    let (mut best, mut best_len, mut total) = (1, 1, 1u64);
    for start in 2..n {
        let mut x = start as u64;
        let mut steps = 0;
        while x >= start as u64 {
            x = if x.is_multiple_of(2) {
                x / 2
            } else {
                3 * x + 1
            };
            steps += 1;
        }
        let len = steps + cache[x as usize];
        cache[start] = len;
        total += u64::from(len);
        if len > best_len {
            best = start;
            best_len = len;
        }
    }
    format!("{n}:{best}:{best_len}:{total}")
}
