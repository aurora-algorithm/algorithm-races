// Race: kruskal (goose)
// Extracted from suite/goose/algo.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

struct Edge { w: i64, u: i64, v: i64 }

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

fn kruskal(n: i64, edges: Edge[:], out: u8[>..]&) {
    sort(edges) { a, b => a.w < b.w };
    var parent: i64[>..] = [];
    var rank:   i64[>..] = [];
    for i in n {
        parent.push(i);
        rank.push(0);
    }
    var count = 0;
    var total = 0;
    for e in edges {
        var ru = find(parent, e.u);
        var rv = find(parent, e.v);
        if ru == rv { continue; }
        if rank[ru] < rank[rv] { swap(ru, rv); }
        parent[rv] = ru;
        if rank[ru] == rank[rv] { rank[ru]++; }
        count++;
        total += e.w;
    }
    format(out, count, ":", total);
}
