// Race: quicksort (goose)
// Extracted from suite/goose/sorts.goose — build and run via suite/run.py

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
