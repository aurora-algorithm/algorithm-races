// Race: bmap-int.go.map (goose)
// Extracted from suite/goose/ways.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

fn way_lap(times: f64[>..]&, t0: i64) { times.push((clock_ns() - t0) / 1e6); }

fn way_map_int(n: i64, repeats: i64, times: f64[>..]&, sum: u8[>..]&) {
    let inserts = ints(n);
    var lookups: i64[>..] = [];
    for k, i in inserts { lookups.push(k ^ (i & 1)); }
    for rep in repeats {
        let t0 = clock_ns();
        var counts = dictionary<i64, i64> {};
        for k in inserts { counts.update(k, 0) { it += 1; }; }
        var hits = 0;
        for k in lookups {
            if counts.contains(k) { hits++; }
        }
        way_lap(times, t0);
        if rep == repeats - 1 { format(sum, counts.count, ":", hits); }
    }
}
