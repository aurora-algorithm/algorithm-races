// Shared inputs and checksums (kernels/SPEC.md section 2): xorshift32, h31, fnv1a.

let SEED:       u32 = 2463534242;
let FNV_OFFSET: u32 = 2166136261;
let FNV_PRIME:  u32 = 16777619;
let ONE_MIB = 1 << 20;

struct XorShift32 { s: u32 }

fn xorshift(seed: u32 = SEED) -> XorShift32 { XorShift32 { seed } }

fn next(r: XorShift32&) -> u32 {
    var s = r.s;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    r.s = s;
    return s;
}

fn ints_from(r: XorShift32&, n: i64) -> i64[>..] {
    var out: i64[>..] = [];
    for i in n { out.push(r.next()); }
    return out;
}

fn ints(n: i64) -> i64[>..] {
    var r = xorshift();
    return ints_from(r, n);
}

fn h31(xs: i64[:]) -> u32 {
    var h: u32 = 0;
    for v in xs { h = h * 31 + v as! u32; }
    return h;
}

fn h31_step(h: u32, v: i64) -> u32 { h * 31 + v as! u32 }

fn fnv1a(data: u8[:], seed: u32 = FNV_OFFSET) -> u32 {
    var h = seed;
    for b in data { h = (h ^ b) * FNV_PRIME; }
    return h;
}

// ints(N) with every value reduced: the grid, terrain and matrix inputs.
fn ints_mod(r: XorShift32&, n: i64, m: u32) -> i64[>..] {
    var out: i64[>..] = [];
    for i in n { out.push(r.next() % m); }
    return out;
}
