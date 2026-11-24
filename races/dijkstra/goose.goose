// Race: dijkstra (goose)
// Extracted from suite/goose/algo.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

struct Visit { dist: i64, node: i64 }

fn dijkstra(n: i64, to: i64[:], weight: i64[:], out: u8[>..]&) {
    let inf = 1 << 62;
    var dist: i64[>..] = [];
    dist.resize(n, inf);
    dist[0] = 0;
    var heap: Visit[>..<] = [];
    heap.heap_push(Visit { 0, 0 }) { a, b => a.dist < b.dist };
    while heap.len > 0 {
        let top = heap.heap_pop() { a, b => a.dist < b.dist };
        if top.dist > dist[top.node] { continue; }
        for e in 4 * top.node..4 * top.node + 4 {
            let v = to[e];
            let nd = top.dist + weight[e];
            if nd < dist[v] {
                dist[v] = nd;
                heap.heap_push(Visit { nd, v }) { a, b => a.dist < b.dist };
            }
        }
    }
    var reached = 0;
    var total = 0;
    var longest = 0;
    for d in dist {
        if d < inf {
            reached++;
            total += d;
            longest = max(longest, d);
        }
    }
    format(out, reached, ":", total, ":", longest);
}
