// Race: lru (goose)
// Extracted from suite/goose/algo.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

// No linked map in the standard library: a dictionary from key to slot, and a doubly linked list of slot
// indexes (prev/next arrays) for the recency order, as the Rust version does with its HashMap and Vec.
fn lru(capacity: i64, keys: i64[:], out: u8[>..]&) {
    var slot_of = dictionary<i64, i64> {};
    var key:  i64[>..] = [];
    var prev: i64[>..] = [];
    var next: i64[>..] = [];
    var head = -1;
    var tail = -1;
    var hits = 0;
    for k, i in keys {
        let found = slot_of.get(k);
        if found {
            let s = found;
            hits++;
            if s != tail {
                let p = prev[s];
                let q = next[s];
                if p >= 0 { next[p] = q; } else { head = q; }
                prev[q] = p;
                prev[s] = tail;
                next[s] = -1;
                next[tail] = s;
                tail = s;
            }
            continue;
        }
        var s = key.len;
        if key.len == capacity {
            s = head;
            slot_of.remove(key[s]);
            head = next[s];
            if head >= 0 { prev[head] = -1; } else { tail = -1; }
            key[s] = k;
        } else {
            key.push(k);
            prev.push(-1);
            next.push(-1);
        }
        slot_of.insert(k, s);
        prev[s] = tail;
        next[s] = -1;
        if tail >= 0 { next[tail] = s; } else { head = s; }
        tail = s;
    }
    var h: u32 = 0;
    var s = head;
    while s >= 0 {
        h = h31_step(h, key[s]);
        s = next[s];
    }
    format(out, hits, ":", keys.len - hits, ":", h);
}
