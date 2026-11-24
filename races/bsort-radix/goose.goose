// Race: bsort-int.go.zermelo.Sort (goose)
// Extracted from suite/goose/ways.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

fn way_lap(times: f64[>..]&, t0: i64) { times.push((clock_ns() - t0) / 1e6); }

// ints(N), or ints(N) sorted ascending (order 1) or descending (order -1); made before timing.
fn way_ints(n: i64, order: i64) -> i64[>..] {
    var a = ints(n);
    if order > 0 { sort(a); }
    if order < 0 { sort(a) { x, y => x > y }; }
    return a;
}

fn way_sort_ints<F>(n: i64, order: i64, repeats: i64, times: f64[>..]&, sum: u8[>..]&) {
    let base = way_ints(n, order);
    for rep in repeats {
        var a = copy(base);
        let t0 = clock_ns();
        F(a);
        way_lap(times, t0);
        if rep == repeats - 1 { format(sum, h31(a)); }
    }
}
