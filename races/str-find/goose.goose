// Race: str-find (goose)
// Extracted from suite/goose/algo.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

fn find(parent: i64[:], x0: i64) -> i64 {
    var root = x0;
    while parent[root] != root { root = parent[root]; }
    var x = x0;
    while parent[x] != root {
        let up = parent[x];
        parent[x] = root;
        x = up;
    }
    return root;
}

struct Matches { count: i64 = 0, first: i64 = -1, last: i64 = -1 }

fn str_find(t: u8[:], pattern: u8[:]) -> Matches {
    var res = Matches {};
    var pos = find(t, pattern);
    while pos >= 0 {
        if res.first < 0 { res.first = pos; }
        res.last = pos;
        res.count++;
        let at = find(t[pos + 1..], pattern);
        pos = if at < 0 { -1 } else { pos + 1 + at };
    }
    return res;
}
