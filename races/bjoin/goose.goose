// Race: bjoin.go.strings.Join (goose)
// Extracted from suite/goose/ways.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

fn way_lap(times: f64[>..]&, t0: i64) { times.push((clock_ns() - t0) / 1e6); }

fn way_join(n: i64, repeats: i64, times: f64[>..]&, sum: u8[>..]&) {
    var parts: u8[..10][>..] = [];
    for v in ints(n) {
        let s .= parts.push([]);
        var digits: u8[>..] = [];
        format(digits, v);
        s.append(digits);
    }
    for rep in repeats {
        let t0 = clock_ns();
        var out: u8[>..] = [];
        for p, i in parts {
            if i > 0 { out.push(','); }
            out.append(p);
        }
        way_lap(times, t0);
        if rep == repeats - 1 { format(sum, out.len, ":", fnv1a(out)); }
    }
}
