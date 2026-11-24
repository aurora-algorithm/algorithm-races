// Race: collatz (goose)
// Extracted from suite/goose/puzzle.goose — build and run via suite/run.py

import std;
import math;
import gen;

fn collatz(n: i64, out: u8[>..]&) {
    var cache: i32[>..] = [];
    cache.resize(n, 0);
    cache[1] = 1;
    var best = 1;
    var best_len: i32 = 1;
    var total = 1;
    for start in 2..n {
        var x = start;
        var steps: i32 = 0;
        while x >= start {
            x = if x % 2 == 0 { x / 2 } else { 3 * x + 1 };
            steps++;
        }
        let len = steps + cache[x];
        cache[start] = len;
        total += len;
        if len > best_len {
            best = start;
            best_len = len;
        }
    }
    format(out, n, ":", best, ":", best_len, ":", total);
}
