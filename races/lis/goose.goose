// Race: lis (goose)
// Extracted from suite/goose/algo.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

fn lis(values: i64[:], out: u8[>..]&) {
    var tails: i64[>..] = [];
    for x in values {
        let i = lower_bound(tails) { it < x };
        if i == tails.len { tails.push(x); } else { tails[i] = x; }
    }
    format(out, tails.len, ":", h31(tails));
}
