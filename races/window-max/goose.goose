// Race: window-max (goose)
// Extracted from suite/goose/puzzle.goose — build and run via suite/run.py

import std;
import math;
import gen;

let WINDOWS = [10, 100, 1000, 10000, 100000];

fn window_max(values: i64[:], out: u8[>..]&) {
    let n = values.len;
    var dq: i64[>..] = [];
    dq.resize(n, 0);
    format(out, n);
    for w in WINDOWS {
        var head = 0;
        var tail = 0;
        var h: u32 = 0;
        for v, i in values {
            while tail > head && values[dq[tail - 1]] <= v { tail--; }
            dq[tail] = i;
            tail++;
            if dq[head] <= i - w { head++; }
            if i >= w - 1 { h = h31_step(h, values[dq[head]]); }
        }
        format(out, ":", h);
    }
}
