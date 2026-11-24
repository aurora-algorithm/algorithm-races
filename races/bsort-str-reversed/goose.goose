// Race: bsort-str-reversed.rust.sort (goose)
// Extracted from suite/goose/ways.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

fn way_lap(times: f64[>..]&, t0: i64) { times.push((clock_ns() - t0) / 1e6); }

fn way_strings(n: i64) -> u8[..16][>..] {
    var r = xorshift();
    var out: u8[..16][>..] = [];
    for i in n {
        let len = 8 + r.next() % 9;
        let s .= out.push([]);
        for j in len { s.push((97 + r.next() % 26) as! u8); }
    }
    return out;
}

fn way_strings_ordered(n: i64, order: i64) -> u8[..16][>..] {
    var a = way_strings(n);
    if order > 0 { sort(a) { x, y => compare(x, y) < 0 }; }
    if order < 0 { sort(a) { x, y => compare(x, y) > 0 }; }
    return a;
}

fn way_sort_strings<F>(n: i64, order: i64, repeats: i64, times: f64[>..]&, sum: u8[>..]&) {
    let base = way_strings_ordered(n, order);
    for rep in repeats {
        var a = copy(base);
        let t0 = clock_ns();
        F(a);
        way_lap(times, t0);
        if rep == repeats - 1 {
            var h = FNV_OFFSET;
            for s in a {
                h = fnv1a(s, h);
                h = fnv1a("\n", h);
            }
            format(sum, h);
        }
    }
}
