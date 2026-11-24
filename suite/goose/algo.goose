// Classic algorithm kernels (kernels/SPEC.md section 12, group `algo`). python/algo.py is the reference.

import std;
import dictionary;
import gen;

let ACGT = "acgt";
let KNAPSACK_ITEMS = 1000;
let PATTERN_LEN = 8;

fn grid(n: i64) -> u8[>..] {
    var r = xorshift();
    var walls: u8[>..] = [];
    for i in n * n { walls.push(if r.next() % 100 < 30 { 1 } else { 0 }); }
    for cell in [0, 1, n, n * n - n - 1, n * n - 2, n * n - 1] { walls[cell] = 0; }
    return walls;
}

// The queue is an array with a head index (Goose has no deque); every cell enters it at most once.
fn bfs_grid(n: i64, walls: u8[:], out: u8[>..]&) {
    var dist: i64[>..] = [];
    dist.resize(n * n, -1);
    var queue: i64[>..] = [];
    dist[0] = 0;
    queue.push(0);
    var head = 0;
    while head < queue.len {
        let cell = queue[head];
        head++;
        let d = dist[cell] + 1;
        let row = cell / n;
        let col = cell % n;
        if row > 0 && walls[cell - n] == 0 && dist[cell - n] < 0 {
            dist[cell - n] = d;
            queue.push(cell - n);
        }
        if row < n - 1 && walls[cell + n] == 0 && dist[cell + n] < 0 {
            dist[cell + n] = d;
            queue.push(cell + n);
        }
        if col > 0 && walls[cell - 1] == 0 && dist[cell - 1] < 0 {
            dist[cell - 1] = d;
            queue.push(cell - 1);
        }
        if col < n - 1 && walls[cell + 1] == 0 && dist[cell + 1] < 0 {
            dist[cell + 1] = d;
            queue.push(cell + 1);
        }
    }
    var reached = 0;
    var total = 0;
    for d in dist {
        if d >= 0 {
            reached++;
            total += d;
        }
    }
    format(out, reached, ":", dist[n * n - 1], ":", total);
}

// Four edges per node u at 4u..4u+3: one to u+1 (a ring, so every node is reachable), three random.
fn graph(n: i64, to: i64[>..]&, weight: i64[>..]&) {
    var r = xorshift();
    for u in n {
        to.push((u + 1) % n);
        weight.push(1 + r.next() % 100);
        for e in 3 {
            to.push(r.next() % n);
            weight.push(1 + r.next() % 100);
        }
    }
}

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

struct Edge { w: i64, u: i64, v: i64 }

fn edge_list(n: i64) -> Edge[>..] {
    var r = xorshift();
    var edges: Edge[>..] = [];
    for i in 4 * n {
        let u = r.next() % n;
        let v = r.next() % n;
        edges.push(Edge { r.next() % 1000000, u, v });
    }
    return edges;
}

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

// Words and prefixes as inline strings: a word is at most 10 letters, a prefix at most 4.
fn words(n: i64, inserted: u8[..10][>..]&, prefixes: u8[..4][>..]&) {
    var r = xorshift();
    for i in n {
        let w .= inserted.push([]);
        let len = 3 + r.next() % 8;
        for j in len { w.push((97 + r.next() % 26) as! u8); }
    }
    for i in n {
        let p .= prefixes.push([]);
        let len = 1 + r.next() % 4;
        for j in len { p.push((97 + r.next() % 26) as! u8); }
    }
}

struct TrieNode { count: i64, children: TrieNode&<u32>?[26] }

fn trie(inserted: u8[..10][:], prefixes: u8[..4][:], out: u8[>..]&) {
    var pool: TrieNode[>..] = [];
    let root .= pool.push(TrieNode { count: 0, .. });
    var nodes = 0;
    for w in inserted {
        var node .= root;
        for ch in w {
            let i = ch - 'a';
            let child = node.children[i];
            if child {
                child.count += 1;
                node .= child;
            } else {
                let fresh .= pool.push(TrieNode { count: 1, .. });
                nodes++;
                node.children[i] .= fresh;
                node .= fresh;
            }
        }
    }
    var total = 0;
    for p in prefixes {
        var node: TrieNode? .= root;
        for ch in p {
            guard node else { break; }
            node .= node.children[ch - 'a'];
        }
        if node { total += node.count; }
    }
    format(out, nodes, ":", total);
}

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

fn dna(r: XorShift32&, n: i64) -> u8[>..] {
    var out: u8[>..] = [];
    for i in n { out.push(ACGT[r.next() % 4]); }
    return out;
}

fn edit_distance(a: u8[:], b: u8[:]) -> i64 {
    let m = b.len;
    var rows: i64[>..] = [];
    for j in m + 1 { rows.push(j); }
    rows.resize(2 * (m + 1), 0);
    var prev: i64[:] = rows[..m + 1];
    var cur:  i64[:] = rows[m + 1..];
    for i in 1..a.len + 1 {
        cur[0] = i;
        let ca = a[i - 1];
        for j in 1..m + 1 {
            let cost = if ca == b[j - 1] { 0 } else { 1 };
            cur[j] = min(min(prev[j] + 1, cur[j - 1] + 1), prev[j - 1] + cost);
        }
        swap(prev, cur);
    }
    return prev[m];
}

struct Item { weight: i64, value: i64 }

fn items() -> Item[>..] {
    var r = xorshift();
    var out: Item[>..] = [];
    for i in KNAPSACK_ITEMS {
        let weight = 1 + r.next() % 1000;
        out.push(Item { weight, 1 + r.next() % 1000 });
    }
    return out;
}

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

fn lis(values: i64[:], out: u8[>..]&) {
    var tails: i64[>..] = [];
    for x in values {
        let i = lower_bound(tails) { it < x };
        if i == tails.len { tails.push(x); } else { tails[i] = x; }
    }
    format(out, tails.len, ":", h31(tails));
}

struct Matches { count: i64 = 0, first: i64 = -1, last: i64 = -1 }

fn kmp(t: u8[:], pattern: u8[:]) -> Matches {
    let m = pattern.len;
    var fail: i64[>..] = [];
    fail.resize(m, 0);
    var k = 0;
    for i in 1..m {
        while k > 0 && pattern[i] != pattern[k] { k = fail[k - 1]; }
        if pattern[i] == pattern[k] { k++; }
        fail[i] = k;
    }
    var res = Matches {};
    k = 0;
    for ch, i in t {
        while k > 0 && ch != pattern[k] { k = fail[k - 1]; }
        if ch == pattern[k] { k++; }
        if k == m {
            res.last = i - m + 1;
            if res.first < 0 { res.first = res.last; }
            res.count++;
            k = fail[k - 1];
        }
    }
    return res;
}

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

recursive fn fib(n: i64) -> i64 {
    if n < 2 { return n; }
    return fib(n - 1) + fib(n - 2);
}

fn nqueens(n: i64) -> i64 {
    var cols: bool[>..] = [];
    var up:   bool[>..] = [];
    var down: bool[>..] = [];
    cols.resize(n, false);
    up.resize(2 * n - 1, false);
    down.resize(2 * n - 1, false);
    recursive fn place(row: i64) -> i64 {
        if row == n { return 1; }
        var count = 0;
        for col in n {
            if !cols[col] && !up[row + col] && !down[row - col + n - 1] {
                cols[col] = true;
                up[row + col] = true;
                down[row - col + n - 1] = true;
                count += place(row + 1);
                cols[col] = false;
                up[row + col] = false;
                down[row - col + n - 1] = false;
            }
        }
        return count;
    }
    return place(0);
}

fn sorted_array(n: i64, a: i64[>..]&, queries: i64[>..]&) {
    var r = xorshift();
    var v = 0;
    for i in n {
        v += 1 + r.next() % 4;
        a.push(v);
    }
    for i in n { queries.push(r.next() % (v + 1)); }
}

fn binary_search(a: i64[:], queries: i64[:], out: u8[>..]&) {
    let n = a.len;
    var hits = 0;
    var total = 0;
    for q in queries {
        var lo = 0;
        var hi = n;
        while lo < hi {
            let mid = (lo + hi) / 2;
            if a[mid] < q { lo = mid + 1; } else { hi = mid; }
        }
        if lo < n && a[lo] == q { hits++; }
        total += lo;
    }
    format(out, hits, ":", total);
}
