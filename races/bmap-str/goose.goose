// Race: bmap-str.go.map (goose)
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

// Keys are slices of the input strings (no copies). Each repeat gets its own copies of the inserts and, apart from
// them, of the lookups (odd ones without the key's last letter), made before the timer, as in the other languages.
fn way_map_str(n: i64, repeats: i64, times: f64[>..]&, sum: u8[>..]&) {
    let inserts = way_strings(n);
    var lookups: u8[..16][>..] = [];
    for i in n {
        let k = inserts[i][..];
        let s .= lookups.push([]);
        s.append(if i % 2 == 0 { k } else { k[..k.len - 1] });
    }
    for rep in repeats {
        let ins = copy(inserts);
        let look = copy(lookups);
        let t0 = clock_ns();
        var counts = dictionary<u8[:], i64> {};
        for i in n { counts.update(ins[i][..], 0) { it += 1; }; }
        var hits = 0;
        for i in n {
            if counts.contains(look[i][..]) { hits++; }
        }
        way_lap(times, t0);
        if rep == repeats - 1 { format(sum, counts.count, ":", hits); }
    }
}
