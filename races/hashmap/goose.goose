// Race: hashmap (goose)
// Extracted from suite/goose/misc.goose — build and run via suite/run.py

import std;
import dictionary;
import regex;
import gen;

fn hashmap(inserts: i64[:], lookups: i64[:], out: u8[>..]&) {
    var counts = dictionary<i64, i64> {};
    for k in inserts { counts.update(k, 0) { it += 1; }; }
    var hits = 0;
    for k in lookups {
        if counts.contains(k) { hits++; }
    }
    format(out, counts.count, ":", hits);
}
