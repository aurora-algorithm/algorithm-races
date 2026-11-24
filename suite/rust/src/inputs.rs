//! Shared inputs and checksums (SPEC.md section 2).

use std::fmt::Write;

const SEED: u32 = 2463534242;
const FNV_OFFSET: u32 = 2166136261;
const FNV_PRIME: u32 = 16777619;

/// Marsaglia's xorshift32; every input starts from a fresh generator.
pub struct XorShift32 {
    state: u32,
}

impl XorShift32 {
    pub fn new() -> Self {
        Self { state: SEED }
    }

    pub fn with_seed(seed: u32) -> Self {
        Self { state: seed }
    }

    pub fn next_u32(&mut self) -> u32 {
        let mut s = self.state;
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        self.state = s;
        s
    }

    pub fn ints(&mut self, n: usize) -> Vec<i64> {
        (0..n).map(|_| i64::from(self.next_u32())).collect()
    }
}

pub fn ints(n: usize) -> Vec<i64> {
    XorShift32::new().ints(n)
}

/// All 64 bits used: two draws per value, high word first, read as a signed i64.
pub fn ints64(n: usize) -> Vec<i64> {
    let mut r = XorShift32::new();
    (0..n)
        .map(|_| {
            let hi = u64::from(r.next_u32());
            let lo = u64::from(r.next_u32());
            ((hi << 32) | lo) as i64
        })
        .collect()
}

/// `h = h * 31 + v` modulo 2^32; only the low 32 bits of each value matter.
pub fn h31(values: impl IntoIterator<Item = i64>) -> u32 {
    values
        .into_iter()
        .fold(0u32, |h, v| h.wrapping_mul(31).wrapping_add(v as u32))
}

pub fn fnv1a(bytes: &[u8]) -> u32 {
    bytes.iter().fold(FNV_OFFSET, |h, &b| {
        (h ^ u32::from(b)).wrapping_mul(FNV_PRIME)
    })
}

pub fn strings(n: usize) -> Vec<String> {
    let mut rng = XorShift32::new();
    (0..n)
        .map(|_| {
            let len = 8 + rng.next_u32() % 9;
            (0..len)
                .map(|_| char::from(b'a' + (rng.next_u32() % 26) as u8))
                .collect()
        })
        .collect()
}

/// One 1 MiB block of generator values (4 bytes each, little-endian), repeated to `size` bytes.
pub fn sha_buffer(size: usize) -> Vec<u8> {
    let mut rng = XorShift32::new();
    let block: Vec<u8> = (0..1 << 18)
        .flat_map(|_| rng.next_u32().to_le_bytes())
        .collect();
    block.repeat(size / block.len())
}

const METHODS: [&str; 4] = ["GET", "POST", "PUT", "DELETE"];
const STATUSES: [u32; 12] = [200, 201, 204, 301, 304, 400, 401, 403, 404, 500, 502, 503];

pub fn log_text(lines: usize) -> String {
    let mut rng = XorShift32::new();
    let mut text = String::new();
    for _ in 0..lines {
        let method = METHODS[(rng.next_u32() % 4) as usize];
        let version = 1 + rng.next_u32() % 3;
        let user = rng.next_u32() % 100000;
        let status = STATUSES[(rng.next_u32() % 12) as usize];
        let ms = rng.next_u32() % 1000;
        writeln!(
            text,
            "{method} /api/v{version}/users/{user} {status} {ms}ms"
        )
        .unwrap();
    }
    text
}

/// A and B, n x n row-major, values `% 10` as doubles, both from one generator.
pub fn matrices(n: usize) -> (Vec<f64>, Vec<f64>) {
    let mut rng = XorShift32::new();
    let mut matrix =
        || -> Vec<f64> { (0..n * n).map(|_| f64::from(rng.next_u32() % 10)).collect() };
    let a = matrix();
    let b = matrix();
    (a, b)
}

/// Keys to insert, and keys to look up: even i is a key in the map, odd i its neighbour.
pub fn hashmap_input(n: usize) -> (Vec<i64>, Vec<i64>) {
    let inserts = ints(n);
    let lookups = inserts
        .iter()
        .enumerate()
        .map(|(i, &k)| k ^ (i as i64 & 1))
        .collect();
    (inserts, lookups)
}
