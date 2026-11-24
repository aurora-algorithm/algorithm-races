// Race: mergesort (goose)
// Extracted from suite/goose/sorts.goose — build and run via suite/run.py

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
