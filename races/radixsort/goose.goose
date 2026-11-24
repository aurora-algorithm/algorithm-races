// Race: radixsort (goose)
// Extracted from suite/goose/sorts.goose — build and run via suite/run.py

// LSD, 8 bits per pass: four counting sorts between `a` and a buffer of n, so the result ends up back in `a`.
fn radix_pass(src: i64[:], dst: i64[:], shift: i64) {
    var count: i64[256] = [0; 256];
    for v in src { count[(v >> shift) & 255]++; }
    var pos: i64[256] = [0; 256];
    var total = 0;
    for d in 256 {
        pos[d] = total;
        total += count[d];
    }
    for v in src {
        let d = (v >> shift) & 255;
        dst[pos[d]] = v;
        pos[d]++;
    }
}

fn radixsort(a: i64[:]) {
    var buf: i64[>..] = [];
    buf.resize(a.len, 0);
    radix_pass(a, buf, 0);
    radix_pass(buf, a, 8);
    radix_pass(a, buf, 16);
    radix_pass(buf, a, 24);
}
