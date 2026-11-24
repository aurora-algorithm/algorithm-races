// Race: mergesort (rust)
// Extracted from suite/rust/src/sorts.rs — build and run via suite/run.py

/// Top-down mergesort of the half-open range [lo, hi), using `aux` as scratch space.
pub fn merge_sort(a: &mut [i64], aux: &mut [i64], lo: usize, hi: usize) {
    if hi - lo < 2 {
        return;
    }
    let mid = lo + (hi - lo) / 2;
    merge_sort(a, aux, lo, mid);
    merge_sort(a, aux, mid, hi);
    merge(a, aux, lo, mid, hi);
}

/// Merges the sorted runs [lo, mid) and [mid, hi) of `a`; stable.
pub fn merge(a: &mut [i64], aux: &mut [i64], lo: usize, mid: usize, hi: usize) {
    aux[lo..hi].copy_from_slice(&a[lo..hi]);
    let (mut i, mut j, mut k) = (lo, mid, lo);
    while i < mid && j < hi {
        if aux[j] < aux[i] {
            a[k] = aux[j];
            j += 1;
        } else {
            a[k] = aux[i];
            i += 1;
        }
        k += 1;
    }
    while i < mid {
        a[k] = aux[i];
        i += 1;
        k += 1;
    }
    while j < hi {
        a[k] = aux[j];
        j += 1;
        k += 1;
    }
}

pub fn run_mergesort(mut a: Vec<i64>) -> Vec<i64> {
    let mut aux = vec![0i64; a.len()];
    let n = a.len();
    merge_sort(&mut a, &mut aux, 0, n);
    a
}
