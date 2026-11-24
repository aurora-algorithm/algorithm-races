//! Sorting kernels, written the same way in every language (SPEC.md section 3).

pub fn std_sort(mut a: Vec<i64>) -> Vec<i64> {
    a.sort();
    a
}

pub fn std_sort_strings(mut a: Vec<String>) -> Vec<String> {
    a.sort();
    a
}

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

fn sift_down(a: &mut [i64], mut root: usize, end: usize) {
    while 2 * root + 1 < end {
        let mut child = 2 * root + 1;
        if child + 1 < end && a[child] < a[child + 1] {
            child += 1;
        }
        if a[root] < a[child] {
            a.swap(root, child);
            root = child;
        } else {
            return;
        }
    }
}

pub fn heapsort(a: &mut [i64]) {
    let n = a.len();
    for start in (0..n / 2).rev() {
        sift_down(a, start, n);
    }
    for end in (1..n).rev() {
        a.swap(0, end);
        sift_down(a, 0, end);
    }
}

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

pub fn run_quicksort(mut a: Vec<i64>) -> Vec<i64> {
    if a.len() > 1 {
        let hi = a.len() - 1;
        quicksort(&mut a, 0, hi);
    }
    a
}

pub fn run_mergesort(mut a: Vec<i64>) -> Vec<i64> {
    let mut aux = vec![0i64; a.len()];
    let n = a.len();
    merge_sort(&mut a, &mut aux, 0, n);
    a
}

pub fn run_heapsort(mut a: Vec<i64>) -> Vec<i64> {
    heapsort(&mut a);
    a
}
