// Race: heapsort (goose)
// Extracted from suite/goose/sorts.goose — build and run via suite/run.py

fn sift_down(a: i64[:], root0: i64, end: i64) {
    var root = root0;
    while 2 * root + 1 < end {
        var child = 2 * root + 1;
        if child + 1 < end && a[child] < a[child + 1] { child++; }
        if a[root] < a[child] {
            let t = a[root];
            a[root] = a[child];
            a[child] = t;
            root = child;
        } else {
            return;
        }
    }
}

fn heapsort(a: i64[:]) {
    let n = a.len;
    var start = n / 2 - 1;
    while start >= 0 {
        sift_down(a, start, n);
        start--;
    }
    var end = n - 1;
    while end > 0 {
        let t = a[0];
        a[0] = a[end];
        a[end] = t;
        sift_down(a, 0, end);
        end--;
    }
}
