// Race: bst-arena (goose)
// Extracted from suite/goose/algo.goose — build and run via suite/run.py

import std;
import dictionary;
import gen;

// The tree lives in one grow-only pool; children are 4-byte self-relative links, null when absent.
struct Node { key: i64, count: i64 = 1, left: Node&<u32>?, right: Node&<u32>? }

fn bst_insert(pool: Node[>..]&, key: i64) -> bool {
    if pool.len == 0 {
        pool.push(Node { key: key });
        return true;
    }
    var cur .= pool[0];
    loop {
        if key < cur.key {
            let l = cur.left;
            if l {
                cur .= l;
                continue;
            }
            cur.left .= pool.push(Node { key: key });
            return true;
        }
        if key > cur.key {
            let r = cur.right;
            if r {
                cur .= r;
                continue;
            }
            cur.right .= pool.push(Node { key: key });
            return true;
        }
        cur.count += 1;
        return false;
    }
}

fn bst_contains(pool: Node[>..]&, key: i64) -> bool {
    guard pool.len > 0 else { return false; }
    var cur .= pool[0];
    loop {
        if key == cur.key { return true; }
        let next = if key < cur.key { cur.left } else { cur.right };
        guard next else { return false; }
        cur .= next;
    }
}

fn in_order_hash(pool: Node[>..]&) -> u32 {
    var h: u32 = 0;
    guard pool.len > 0 else { return h; }
    var stack: Node&[>..<] = [];
    var node:  Node? .= pool[0];
    while stack.len > 0 || node != null {
        while node {
            stack.push(node);
            node .= node.left;
        }
        let top: Node& = stack.pop();
        h = h31_step(h, top.key);
        node .= top.right;
    }
    return h;
}

fn bst(inserts: i64[:], lookups: i64[:], out: u8[>..]&) {
    var pool: Node[>..] = [];
    var size = 0;
    for k in inserts {
        if bst_insert(pool, k) { size++; }
    }
    var hits = 0;
    for k in lookups {
        if bst_contains(pool, k) { hits++; }
    }
    format(out, size, ":", hits, ":", in_order_hash(pool));
}
