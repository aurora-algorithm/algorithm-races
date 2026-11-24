//! Classic algorithm kernels (../SPEC.md section 12, group `algo`).

use std::cmp::{Ordering, Reverse};
use std::collections::{BTreeMap, BinaryHeap, HashMap, VecDeque};

use crate::inputs::{XorShift32, h31, hashmap_input, ints};
use crate::work::Outcome;
use crate::{Args, measure};

const ACGT: &[u8; 4] = b"acgt";
const KNAPSACK_ITEMS: usize = 1000;
const PATTERN_LEN: usize = 8;
const WORDS: [&str; 64] = [
    "the", "be", "to", "of", "and", "a", "in", "that", "have", "i", "it", "for", "not", "on",
    "with", "he", "as", "you", "do", "at", "this", "but", "his", "by", "from", "they", "we", "say",
    "her", "she", "or", "an", "will", "my", "one", "all", "would", "there", "their", "what", "so",
    "up", "out", "if", "about", "who", "get", "which", "go", "me", "when", "make", "can", "like",
    "time", "no", "just", "him", "know", "take", "people", "into", "year", "your",
];

pub fn run(args: &Args) -> Result<Option<Outcome>, String> {
    let (n, repeats) = (args.size, args.repeats);
    let (checksum, times) = match args.kernel.as_str() {
        "bfs-grid" => {
            let walls = grid(n);
            measure(repeats, || &walls, |w| bfs_grid(n, w))
        }
        "dijkstra" => {
            let (to, weight) = graph(n);
            measure(repeats, || (&to, &weight), |(t, w)| dijkstra(n, t, w))
        }
        "kruskal" => {
            let edges = edge_list(n);
            measure(repeats, || edges.clone(), |e| kruskal(n, e))
        }
        "bst" => {
            let (inserts, lookups) = hashmap_input(n);
            measure(
                repeats,
                || (),
                |()| Bst::build(&inserts, &lookups).checksum(),
            )
        }
        "ordered-map" => {
            let (inserts, lookups) = hashmap_input(n);
            measure(repeats, || (), |()| ordered_map(&inserts, &lookups))
        }
        "trie" => {
            let (inserted, prefixes) = words(n);
            measure(repeats, || (), |()| trie(&inserted, &prefixes))
        }
        "lru" => {
            let (capacity, keys) = lru_keys(n);
            measure(repeats, || &keys, |k| lru(capacity, k))
        }
        "edit-distance" => {
            let ab = dna(n, 2);
            measure(repeats, || &ab, |ab| edit_distance(&ab[0], &ab[1]))
        }
        "knapsack" => {
            let things = items();
            measure(repeats, || &things, |t| knapsack(n, t))
        }
        "lis" => {
            let values = ints(n);
            measure(repeats, || &values, |v| lis(v))
        }
        "kmp" => {
            let (t, p) = text(n);
            let (m, times) = measure(repeats, || (), |()| kmp(&t, &p));
            (m.to_string(), times)
        }
        "str-find" => {
            let (t, p) = text(n);
            let (m, times) = measure(repeats, || (), |()| str_find(&t, &p));
            (m.to_string(), times)
        }
        "kmp-en" => {
            let (t, p) = text_en(n);
            let (m, times) = measure(repeats, || (), |()| kmp(&t, &p));
            (m.to_string(), times)
        }
        "str-find-en" => {
            let (t, p) = text_en(n);
            let (m, times) = measure(repeats, || (), |()| str_find(&t, &p));
            (m.to_string(), times)
        }
        "fib" => {
            let depth = u32::try_from(n).map_err(|_| format!("--size {n} is too large"))?;
            measure(repeats, || depth, |d| fib(d).to_string())
        }
        "nqueens" => measure(repeats, || n, |n| nqueens(n).to_string()),
        "binary-search" => {
            let (a, queries) = sorted_array(n);
            measure(repeats, || (), |()| binary_search(&a, &queries))
        }
        _ => return Ok(None),
    };
    Ok(Some((checksum, times, None)))
}

fn grid(n: usize) -> Vec<bool> {
    let mut rng = XorShift32::new();
    let mut walls: Vec<bool> = (0..n * n).map(|_| rng.next_u32() % 100 < 30).collect();
    for cell in [0, 1, n, n * n - n - 1, n * n - 2, n * n - 1] {
        walls[cell] = false;
    }
    walls
}

fn bfs_grid(n: usize, walls: &[bool]) -> String {
    let mut dist = vec![-1i32; n * n];
    dist[0] = 0;
    let mut queue = VecDeque::from([0usize]);
    while let Some(cell) = queue.pop_front() {
        let d = dist[cell] + 1;
        let (row, col) = (cell / n, cell % n);
        let neighbours = [
            (cell.wrapping_sub(n), row > 0),
            (cell + n, row < n - 1),
            (cell.wrapping_sub(1), col > 0),
            (cell + 1, col < n - 1),
        ];
        for (next, ok) in neighbours {
            if ok && !walls[next] && dist[next] < 0 {
                dist[next] = d;
                queue.push_back(next);
            }
        }
    }
    let reached = dist.iter().filter(|&&d| d >= 0).count();
    let total: i64 = dist
        .iter()
        .filter(|&&d| d >= 0)
        .map(|&d| i64::from(d))
        .sum();
    format!("{reached}:{}:{total}", dist[n * n - 1])
}

/// Four edges per node u at 4u..4u+3: one to u+1 (a ring, so every node is reachable), three random.
fn graph(n: usize) -> (Vec<usize>, Vec<u64>) {
    let mut rng = XorShift32::new();
    let mut to = vec![0; 4 * n];
    let mut weight = vec![0; 4 * n];
    for u in 0..n {
        to[4 * u] = (u + 1) % n;
        weight[4 * u] = 1 + u64::from(rng.next_u32() % 100);
        for e in 4 * u + 1..4 * u + 4 {
            to[e] = rng.next_u32() as usize % n;
            weight[e] = 1 + u64::from(rng.next_u32() % 100);
        }
    }
    (to, weight)
}

/// A heap entry ordered by distance only, as Go's and Goose's heaps compare it (a tuple would compare the node too).
struct Visit {
    dist: u64,
    node: usize,
}

impl PartialEq for Visit {
    fn eq(&self, other: &Self) -> bool {
        self.dist == other.dist
    }
}

impl Eq for Visit {}

impl PartialOrd for Visit {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for Visit {
    fn cmp(&self, other: &Self) -> Ordering {
        self.dist.cmp(&other.dist)
    }
}

fn dijkstra(n: usize, to: &[usize], weight: &[u64]) -> String {
    let mut dist = vec![u64::MAX; n];
    dist[0] = 0;
    let mut heap = BinaryHeap::from([Reverse(Visit { dist: 0, node: 0 })]);
    while let Some(Reverse(Visit { dist: d, node: u })) = heap.pop() {
        if d > dist[u] {
            continue;
        }
        for e in 4 * u..4 * u + 4 {
            let v = to[e];
            let nd = d + weight[e];
            if nd < dist[v] {
                dist[v] = nd;
                heap.push(Reverse(Visit { dist: nd, node: v }));
            }
        }
    }
    let reached: Vec<u64> = dist.into_iter().filter(|&d| d < u64::MAX).collect();
    let total: u64 = reached.iter().sum();
    let max = reached.iter().max().copied().unwrap_or(0);
    format!("{}:{total}:{max}", reached.len())
}

#[derive(Clone, Copy)]
struct Edge {
    w: u32,
    u: usize,
    v: usize,
}

fn edge_list(n: usize) -> Vec<Edge> {
    let mut rng = XorShift32::new();
    (0..4 * n)
        .map(|_| {
            let u = rng.next_u32() as usize % n;
            let v = rng.next_u32() as usize % n;
            Edge {
                w: rng.next_u32() % 1_000_000,
                u,
                v,
            }
        })
        .collect()
}

fn find(parent: &mut [usize], mut x: usize) -> usize {
    let mut root = x;
    while parent[root] != root {
        root = parent[root];
    }
    while parent[x] != root {
        let next = parent[x];
        parent[x] = root;
        x = next;
    }
    root
}

fn kruskal(n: usize, mut edges: Vec<Edge>) -> String {
    edges.sort_unstable_by_key(|e| e.w);
    let mut parent: Vec<usize> = (0..n).collect();
    let mut rank = vec![0u8; n];
    let (mut count, mut total) = (0u64, 0u64);
    for e in &edges {
        let mut ru = find(&mut parent, e.u);
        let mut rv = find(&mut parent, e.v);
        if ru == rv {
            continue;
        }
        if rank[ru] < rank[rv] {
            std::mem::swap(&mut ru, &mut rv);
        }
        parent[rv] = ru;
        if rank[ru] == rank[rv] {
            rank[ru] += 1;
        }
        count += 1;
        total += u64::from(e.w);
    }
    format!("{count}:{total}")
}

struct Node {
    key: i64,
    count: u32,
    left: Option<Box<Node>>,
    right: Option<Box<Node>>,
}

struct Bst {
    root: Option<Box<Node>>,
    size: usize,
    hits: usize,
}

impl Bst {
    fn build(inserts: &[i64], lookups: &[i64]) -> Self {
        let mut tree = Bst {
            root: None,
            size: 0,
            hits: 0,
        };
        for &k in inserts {
            tree.insert(k);
        }
        tree.hits = lookups.iter().filter(|&&k| tree.contains(k)).count();
        tree
    }

    fn insert(&mut self, key: i64) {
        let mut slot = &mut self.root;
        while let Some(node) = slot {
            slot = match key.cmp(&node.key) {
                Ordering::Less => &mut node.left,
                Ordering::Greater => &mut node.right,
                Ordering::Equal => {
                    node.count += 1;
                    return;
                }
            };
        }
        *slot = Some(Box::new(Node {
            key,
            count: 1,
            left: None,
            right: None,
        }));
        self.size += 1;
    }

    fn contains(&self, key: i64) -> bool {
        let mut node = self.root.as_deref();
        while let Some(n) = node {
            node = match key.cmp(&n.key) {
                Ordering::Less => n.left.as_deref(),
                Ordering::Greater => n.right.as_deref(),
                Ordering::Equal => return true,
            };
        }
        false
    }

    fn checksum(&self) -> String {
        let mut keys = Vec::with_capacity(self.size);
        let mut stack = Vec::new();
        let mut node = self.root.as_deref();
        while node.is_some() || !stack.is_empty() {
            while let Some(n) = node {
                stack.push(n);
                node = n.left.as_deref();
            }
            let n = stack
                .pop()
                .expect("the loop condition keeps the stack non-empty");
            keys.push(n.key);
            node = n.right.as_deref();
        }
        format!("{}:{}:{}", self.size, self.hits, h31(keys))
    }
}

fn ordered_map(inserts: &[i64], lookups: &[i64]) -> String {
    let mut counts = BTreeMap::new();
    for &k in inserts {
        *counts.entry(k).or_insert(0) += 1;
    }
    let hits = lookups.iter().filter(|k| counts.contains_key(k)).count();
    format!("{}:{hits}:{}", counts.len(), h31(counts.keys().copied()))
}

fn word(rng: &mut XorShift32, min_len: u32, spread: u32) -> Vec<u8> {
    let len = min_len + rng.next_u32() % spread;
    (0..len)
        .map(|_| b'a' + (rng.next_u32() % 26) as u8)
        .collect()
}

pub fn words(n: usize) -> (Vec<Vec<u8>>, Vec<Vec<u8>>) {
    let mut rng = XorShift32::new();
    let inserted = (0..n).map(|_| word(&mut rng, 3, 8)).collect();
    let prefixes = (0..n).map(|_| word(&mut rng, 1, 4)).collect();
    (inserted, prefixes)
}

#[derive(Default)]
struct TrieNode {
    count: u64,
    children: [Option<Box<TrieNode>>; 26],
}

fn trie(inserted: &[Vec<u8>], prefixes: &[Vec<u8>]) -> String {
    let mut root = TrieNode::default();
    let mut nodes = 0;
    for w in inserted {
        let mut node = &mut root;
        for &ch in w {
            node = node.children[usize::from(ch - b'a')].get_or_insert_with(|| {
                nodes += 1;
                Box::default()
            });
            node.count += 1;
        }
    }
    let mut total = 0;
    for p in prefixes {
        let mut node = Some(&root);
        for &ch in p {
            node = node.and_then(|n| n.children[usize::from(ch - b'a')].as_deref());
        }
        total += node.map_or(0, |n| n.count);
    }
    format!("{nodes}:{total}")
}

fn lru_keys(n: usize) -> (usize, Vec<u32>) {
    let capacity = (n / 100).max(1);
    let mut rng = XorShift32::new();
    let keys = (0..n)
        .map(|_| rng.next_u32() % (2 * capacity as u32))
        .collect();
    (capacity, keys)
}

const NIL: usize = usize::MAX;

/// One cache entry in a doubly linked list kept in a Vec (indices instead of pointers).
struct Entry {
    key: u32,
    prev: usize,
    next: usize,
}

/// std has no linked hash map: a HashMap from key to slot plus the list of slots, most recent at `head`.
struct Lru {
    slots: HashMap<u32, usize>,
    entries: Vec<Entry>,
    head: usize,
    tail: usize,
}

impl Lru {
    fn unlink(&mut self, i: usize) {
        let (prev, next) = (self.entries[i].prev, self.entries[i].next);
        match prev {
            NIL => self.head = next,
            p => self.entries[p].next = next,
        }
        match next {
            NIL => self.tail = prev,
            n => self.entries[n].prev = prev,
        }
    }

    fn push_front(&mut self, i: usize) {
        self.entries[i].prev = NIL;
        self.entries[i].next = self.head;
        match self.head {
            NIL => self.tail = i,
            h => self.entries[h].prev = i,
        }
        self.head = i;
    }

    fn keys_oldest_first(&self) -> Vec<i64> {
        let mut keys = Vec::with_capacity(self.entries.len());
        let mut i = self.tail;
        while i != NIL {
            keys.push(i64::from(self.entries[i].key));
            i = self.entries[i].prev;
        }
        keys
    }
}

fn lru(capacity: usize, keys: &[u32]) -> String {
    let mut cache = Lru {
        slots: HashMap::with_capacity(capacity),
        entries: Vec::with_capacity(capacity),
        head: NIL,
        tail: NIL,
    };
    let mut hits = 0;
    for &k in keys {
        if let Some(&i) = cache.slots.get(&k) {
            cache.unlink(i);
            cache.push_front(i);
            hits += 1;
            continue;
        }
        let i = if cache.entries.len() == capacity {
            let oldest = cache.tail;
            cache.unlink(oldest);
            cache.slots.remove(&cache.entries[oldest].key);
            cache.entries[oldest].key = k;
            oldest
        } else {
            cache.entries.push(Entry {
                key: k,
                prev: NIL,
                next: NIL,
            });
            cache.entries.len() - 1
        };
        cache.slots.insert(k, i);
        cache.push_front(i);
    }
    let misses = keys.len() - hits;
    format!("{hits}:{misses}:{}", h31(cache.keys_oldest_first()))
}

fn dna(length: usize, count: usize) -> Vec<Vec<u8>> {
    let mut rng = XorShift32::new();
    (0..count)
        .map(|_| {
            (0..length)
                .map(|_| ACGT[(rng.next_u32() % 4) as usize])
                .collect()
        })
        .collect()
}

fn edit_distance(a: &[u8], b: &[u8]) -> String {
    let mut prev: Vec<usize> = (0..=b.len()).collect();
    let mut cur = vec![0; b.len() + 1];
    for (i, &ca) in a.iter().enumerate() {
        cur[0] = i + 1;
        for (j, &cb) in b.iter().enumerate() {
            let cost = usize::from(ca != cb);
            cur[j + 1] = (prev[j + 1] + 1).min(cur[j] + 1).min(prev[j] + cost);
        }
        std::mem::swap(&mut prev, &mut cur);
    }
    prev[b.len()].to_string()
}

fn items() -> Vec<(usize, u64)> {
    let mut rng = XorShift32::new();
    (0..KNAPSACK_ITEMS)
        .map(|_| {
            let weight = 1 + (rng.next_u32() % 1000) as usize;
            (weight, 1 + u64::from(rng.next_u32() % 1000))
        })
        .collect()
}

fn knapsack(capacity: usize, things: &[(usize, u64)]) -> String {
    let mut best = vec![0u64; capacity + 1];
    for &(weight, value) in things {
        for c in (weight..capacity + 1).rev() {
            if best[c - weight] + value > best[c] {
                best[c] = best[c - weight] + value;
            }
        }
    }
    format!("{}:{}", best[capacity], h31(best.iter().map(|&v| v as i64)))
}

fn lis(values: &[i64]) -> String {
    let mut tails: Vec<i64> = Vec::new();
    for &x in values {
        let i = tails.partition_point(|&t| t < x);
        if i == tails.len() {
            tails.push(x);
        } else {
            tails[i] = x;
        }
    }
    format!("{}:{}", tails.len(), h31(tails.iter().copied()))
}

fn text(n: usize) -> (String, String) {
    let bytes = dna(n, 1).remove(0);
    let t = String::from_utf8(bytes).expect("acgt is ASCII");
    let p = t[n / 2..n / 2 + PATTERN_LEN].to_string();
    (t, p)
}

fn english(n: usize) -> String {
    let mut rng = XorShift32::new();
    let mut t = String::with_capacity(n + 16);
    while t.len() < n {
        let a = rng.next_u32() % 64;
        let b = rng.next_u32() % 64;
        t.push_str(WORDS[a.min(b) as usize]);
        t.push_str(match rng.next_u32() % 16 {
            0 => ". ",
            1 => ", ",
            _ => " ",
        });
    }
    t.truncate(n);
    t
}

fn text_en(n: usize) -> (String, String) {
    let t = english(n);
    let p = t[n / 2..n / 2 + PATTERN_LEN].to_string();
    (t, p)
}

struct Matches {
    count: usize,
    first: i64,
    last: i64,
}

impl Matches {
    fn new() -> Self {
        Matches {
            count: 0,
            first: -1,
            last: -1,
        }
    }

    fn add(&mut self, pos: usize) {
        let pos = pos as i64;
        if self.first < 0 {
            self.first = pos;
        }
        self.last = pos;
        self.count += 1;
    }
}

impl std::fmt::Display for Matches {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}:{}:{}", self.count, self.first, self.last)
    }
}

fn kmp(t: &str, pattern: &str) -> Matches {
    let (t, p) = (t.as_bytes(), pattern.as_bytes());
    let m = p.len();
    let mut fail = vec![0; m];
    let mut k = 0;
    for i in 1..m {
        while k > 0 && p[i] != p[k] {
            k = fail[k - 1];
        }
        if p[i] == p[k] {
            k += 1;
        }
        fail[i] = k;
    }
    let mut found = Matches::new();
    k = 0;
    for (i, &ch) in t.iter().enumerate() {
        while k > 0 && ch != p[k] {
            k = fail[k - 1];
        }
        if ch == p[k] {
            k += 1;
        }
        if k == m {
            found.add(i + 1 - m);
            k = fail[k - 1];
        }
    }
    found
}

fn str_find(t: &str, pattern: &str) -> Matches {
    let mut found = Matches::new();
    let mut start = 0;
    while let Some(offset) = t[start..].find(pattern) {
        found.add(start + offset);
        start += offset + 1;
    }
    found
}

fn fib(n: u32) -> u64 {
    if n < 2 {
        u64::from(n)
    } else {
        fib(n - 1) + fib(n - 2)
    }
}

struct Board {
    n: usize,
    cols: Vec<bool>,
    up: Vec<bool>,
    down: Vec<bool>,
}

impl Board {
    fn place(&mut self, row: usize) -> u64 {
        if row == self.n {
            return 1;
        }
        let mut count = 0;
        for col in 0..self.n {
            let (u, d) = (row + col, row + self.n - 1 - col);
            if !self.cols[col] && !self.up[u] && !self.down[d] {
                (self.cols[col], self.up[u], self.down[d]) = (true, true, true);
                count += self.place(row + 1);
                (self.cols[col], self.up[u], self.down[d]) = (false, false, false);
            }
        }
        count
    }
}

fn nqueens(n: usize) -> u64 {
    let mut board = Board {
        n,
        cols: vec![false; n],
        up: vec![false; 2 * n - 1],
        down: vec![false; 2 * n - 1],
    };
    board.place(0)
}

fn sorted_array(n: usize) -> (Vec<i64>, Vec<i64>) {
    let mut rng = XorShift32::new();
    let mut v = 0;
    let a: Vec<i64> = (0..n)
        .map(|_| {
            v += 1 + i64::from(rng.next_u32() % 4);
            v
        })
        .collect();
    let queries = (0..n)
        .map(|_| i64::from(rng.next_u32()) % (v + 1))
        .collect();
    (a, queries)
}

fn binary_search(a: &[i64], queries: &[i64]) -> String {
    let n = a.len();
    let (mut hits, mut total) = (0u64, 0u64);
    for &q in queries {
        let (mut lo, mut hi) = (0, n);
        while lo < hi {
            let mid = (lo + hi) / 2;
            if a[mid] < q {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        if lo < n && a[lo] == q {
            hits += 1;
        }
        total += lo as u64;
    }
    format!("{hits}:{total}")
}
