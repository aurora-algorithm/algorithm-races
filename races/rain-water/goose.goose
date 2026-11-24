// Race: rain-water (goose)
// Extracted from suite/goose/puzzle.goose — build and run via suite/run.py

import std;
import math;
import gen;

fn rain_water(heights: i64[:], out: u8[>..]&) {
    let n = heights.len;
    let side = isqrt(n);
    var seen: bool[>..] = [];
    seen.resize(n, false);
    var heap: i64[>..<] = [];
    for idx in n {
        let r = idx / side;
        let c = idx % side;
        if r == 0 || r == side - 1 || c == 0 || c == side - 1 {
            seen[idx] = true;
            heap.push(heights[idx] * n + idx);
        }
    }
    heap.make_heap();
    var water = 0;
    var wet = 0;
    while heap.len > 0 {
        let top = heap.heap_pop();
        let level = top / n;
        let idx = top % n;
        let r = idx / side;
        let c = idx % side;
        for d in 4 {
            let nr = r + [-1, 1, 0, 0][d];
            let nc = c + [0, 0, -1, 1][d];
            if nr < 0 || nr >= side || nc < 0 || nc >= side { continue; }
            let nidx = nr * side + nc;
            if seen[nidx] { continue; }
            seen[nidx] = true;
            var h = heights[nidx];
            if h < level {
                water += level - h;
                wet++;
                h = level;
            }
            heap.heap_push(h * n + nidx);
        }
    }
    format(out, side, ":", water, ":", wet);
}
