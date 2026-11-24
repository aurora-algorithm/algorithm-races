// Sorting kernels, written the same way in every language (kernels/SPEC.md section 3).

recursive fn quicksort(a: i64[:], lo0: i64, hi0: i64) {
    var lo = lo0;
    var hi = hi0;
    while lo < hi {
        let pivot = a[lo + (hi - lo) / 2];
        var i = lo - 1;
        var j = hi + 1;
        loop {
            i++;
            while a[i] < pivot { i++; }
            j--;
            while a[j] > pivot { j--; }
            if i >= j { break; }
            let t = a[i];
            a[i] = a[j];
            a[j] = t;
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

fn run_quicksort(a: i64[:]) { quicksort(a, 0, a.len - 1); }

fn merge(a: i64[:], aux: i64[:], lo: i64, mid: i64, hi: i64) {
    copy_into(aux[lo..hi], a[lo..hi]);
    var i = lo;
    var j = mid;
    var k = lo;
    while i < mid && j < hi {
        if aux[j] < aux[i] {
            a[k] = aux[j];
            j++;
        } else {
            a[k] = aux[i];
            i++;
        }
        k++;
    }
    while i < mid {
        a[k] = aux[i];
        i++;
        k++;
    }
    while j < hi {
        a[k] = aux[j];
        j++;
        k++;
    }
}

recursive fn merge_sort(a: i64[:], aux: i64[:], lo: i64, hi: i64) {
    if hi - lo < 2 { return; }
    let mid = lo + (hi - lo) / 2;
    merge_sort(a, aux, lo, mid);
    merge_sort(a, aux, mid, hi);
    merge(a, aux, lo, mid, hi);
}

fn run_mergesort(a: i64[:]) {
    var aux: i64[>..] = [];
    aux.resize(a.len, 0);
    merge_sort(a, aux, 0, a.len);
}

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
