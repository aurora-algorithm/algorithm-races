// Race: heapsort (rust)
// Extracted from suite/rust/src/sorts.rs — build and run via suite/run.py

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

pub fn run_heapsort(mut a: Vec<i64>) -> Vec<i64> {
    heapsort(&mut a);
    a
}
