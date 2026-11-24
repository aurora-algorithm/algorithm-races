// Race: radixsort (rust)
// Extracted from suite/rust/src/sorts.rs — build and run via suite/run.py

/// LSD radix sort, 8 bits per pass, for values below 2^32.
pub fn radix_sort(mut a: Vec<i64>) -> Vec<i64> {
    let mut buf = vec![0i64; a.len()];
    for shift in [0, 8, 16, 24] {
        let mut count = [0usize; 256];
        for &v in &a {
            count[((v >> shift) & 255) as usize] += 1;
        }
        let mut pos = [0usize; 256];
        let mut total = 0;
        for d in 0..256 {
            pos[d] = total;
            total += count[d];
        }
        for &v in &a {
            let d = ((v >> shift) & 255) as usize;
            buf[pos[d]] = v;
            pos[d] += 1;
        }
        std::mem::swap(&mut a, &mut buf);
    }
    a
}
