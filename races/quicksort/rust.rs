// Race: quicksort (rust)
// Extracted from suite/rust/src/sorts.rs — build and run via suite/run.py

/// Hoare partition with the middle element as pivot; inclusive bounds.
/// Recurses into the smaller side and loops on the larger one.
pub fn quicksort(a: &mut [i64], mut lo: usize, mut hi: usize) {
    while lo < hi {
        let pivot = a[lo + (hi - lo) / 2];
        let mut i = lo;
        let mut j = hi;
        loop {
            while a[i] < pivot {
                i += 1;
            }
            while a[j] > pivot {
                j -= 1;
            }
            if i >= j {
                break;
            }
            a.swap(i, j);
            i += 1;
            j -= 1;
        }
        if j - lo < hi - j {
            quicksort(a, lo, j);
            lo = j + 1;
        } else {
            quicksort(a, j + 1, hi);
            hi = j;
        }
    }
}

pub fn run_quicksort(mut a: Vec<i64>) -> Vec<i64> {
    if a.len() > 1 {
        let hi = a.len() - 1;
        quicksort(&mut a, 0, hi);
    }
    a
}
