// Race: skiplist (goose)
// Extracted from suite/goose/puzzle.goose — build and run via suite/run.py

import std;
import math;
import gen;

let SKIP_MAX_LEVEL = 24;

let SKIP_LEVEL_SEED: u32 = 88675123;

// Nodes in flat arrays with index links, as the Rust version keeps them in a Vec: node i's forward link at
// level l is next[base[i] + l]. Node 0 is the head, with all 24 levels. The arrays are the caller's (empty on
// entry), so skiplist_walk can hash level 0 after the timer.
fn skiplist(keys: i64[:], lookups: i64[:], key: i64[>..]&, base: i64[>..]&, next: i64[>..]&, out: u8[>..]&) {
    var r = xorshift(SKIP_LEVEL_SEED);
    key.push(-1);
    base.push(0);
    next.resize(SKIP_MAX_LEVEL, -1);
    var level = 1;
    var update: i64[24] = [0; 24];
    var heights = 0;
    for k in keys {
        var x = 0;
        var i = level - 1;
        while i >= 0 {
            var nxt = next[base[x] + i];
            while nxt >= 0 && key[nxt] < k {
                x = nxt;
                nxt = next[base[x] + i];
            }
            update[i] = x;
            i--;
        }
        let first = next[base[x]];
        if first >= 0 && key[first] == k { continue; }
        var height = 1;
        while height < SKIP_MAX_LEVEL && r.next() & 1 != 0 { height++; }
        for l in level..height { update[l] = 0; }
        level = max(level, height);
        let node = key.len;
        key.push(k);
        base.push(next.len);
        for l in height {
            next.push(next[base[update[l]] + l]);
            next[base[update[l]] + l] = node;
        }
        heights += height;
    }
    var hits = 0;
    for k in lookups {
        var x = 0;
        var i = level - 1;
        while i >= 0 {
            var nxt = next[base[x] + i];
            while nxt >= 0 && key[nxt] < k {
                x = nxt;
                nxt = next[base[x] + i];
            }
            i--;
        }
        let first = next[base[x]];
        if first >= 0 && key[first] == k { hits++; }
    }
    format(out, keys.len, ":", hits, ":", heights, ":");
}
