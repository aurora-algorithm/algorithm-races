// Race: knapsack (goose)
// Extracted from suite/goose/algo.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

struct Item { weight: i64, value: i64 }

fn knapsack(capacity: i64, things: Item[:], out: u8[>..]&) {
    var best: i64[>..] = [];
    best.resize(capacity + 1, 0);
    for t in things {
        var c = capacity;
        while c >= t.weight {
            let with = best[c - t.weight] + t.value;
            if with > best[c] { best[c] = with; }
            c--;
        }
    }
    format(out, best[capacity], ":", h31(best));
}
