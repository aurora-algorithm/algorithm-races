// Race: binary-search (goose)
// Extracted from suite/goose/algo.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

fn sorted_array(n: i64, a: i64[>..]&, queries: i64[>..]&) {
    var r = xorshift();
    var v = 0;
    for i in n {
        v += 1 + r.next() % 4;
        a.push(v);
    }
    for i in n { queries.push(r.next() % (v + 1)); }
}

fn binary_search(a: i64[:], queries: i64[:], out: u8[>..]&) {
    let n = a.len;
    var hits = 0;
    var total = 0;
    for q in queries {
        var lo = 0;
        var hi = n;
        while lo < hi {
            let mid = (lo + hi) / 2;
            if a[mid] < q { lo = mid + 1; } else { hi = mid; }
        }
        if lo < n && a[lo] == q { hits++; }
        total += lo;
    }
    format(out, hits, ":", total);
}
