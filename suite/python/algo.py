"""Classic algorithm kernels (kernels/SPEC.md section 12, group `algo`)."""

import heapq
import time
from bisect import bisect_left
from collections import OrderedDict, deque
from collections.abc import Iterator
from operator import itemgetter

from gen import XorShift32, h31

ACGT = "acgt"
KNAPSACK_ITEMS = 1000
PATTERN_LEN = 8
# The 64 most common English words (Oxford English Corpus order, as listed by Wikipedia's "Most common words in
# English"); english() picks min of two draws, so earlier words come up more often (roughly Zipf-like).
WORDS = [
    "the",
    "be",
    "to",
    "of",
    "and",
    "a",
    "in",
    "that",
    "have",
    "i",
    "it",
    "for",
    "not",
    "on",
    "with",
    "he",
    "as",
    "you",
    "do",
    "at",
    "this",
    "but",
    "his",
    "by",
    "from",
    "they",
    "we",
    "say",
    "her",
    "she",
    "or",
    "an",
    "will",
    "my",
    "one",
    "all",
    "would",
    "there",
    "their",
    "what",
    "so",
    "up",
    "out",
    "if",
    "about",
    "who",
    "get",
    "which",
    "go",
    "me",
    "when",
    "make",
    "can",
    "like",
    "time",
    "no",
    "just",
    "him",
    "know",
    "take",
    "people",
    "into",
    "year",
    "your",
]
CHUNK = 1 << 20


def chunks(rng: XorShift32, n: int) -> Iterator[list[int]]:
    """The next n generator values, a million at a time (setup speed only; the values are the same)."""
    for start in range(0, n, CHUNK):
        yield rng.ints(min(CHUNK, n - start))


def grid(n: int) -> bytearray:
    walls = bytearray()
    for part in chunks(XorShift32(), n * n):
        walls += bytes(1 if v % 100 < 30 else 0 for v in part)
    for cell in (0, 1, n, n * n - n - 1, n * n - 2, n * n - 1):
        walls[cell] = 0
    return walls


def bfs_grid(n: int, walls: bytearray) -> str:
    dist = [-1] * (n * n)
    dist[0] = 0
    queue = deque([0])
    while queue:
        cell = queue.popleft()
        d = dist[cell] + 1
        row, col = divmod(cell, n)
        for nxt, ok in ((cell - n, row > 0), (cell + n, row < n - 1), (cell - 1, col > 0), (cell + 1, col < n - 1)):
            if ok and not walls[nxt] and dist[nxt] < 0:
                dist[nxt] = d
                queue.append(nxt)
    reached = [d for d in dist if d >= 0]
    return f"{len(reached)}:{dist[-1]}:{sum(reached)}"


def graph(n: int) -> tuple[list[int], list[int]]:
    """Four edges per node u at 4u..4u+3: one to u+1 (a ring, so every node is reachable), three random."""
    rng = XorShift32()
    to = [0] * (4 * n)
    weight = [0] * (4 * n)
    for u in range(n):
        to[4 * u] = (u + 1) % n
        weight[4 * u] = 1 + rng.next() % 100
        for e in range(4 * u + 1, 4 * u + 4):
            to[e] = rng.next() % n
            weight[e] = 1 + rng.next() % 100
    return to, weight


def dijkstra(n: int, to: list[int], weight: list[int]) -> str:
    inf = 1 << 62
    dist = [inf] * n
    dist[0] = 0
    heap = [(0, 0)]
    while heap:
        d, u = heapq.heappop(heap)
        if d > dist[u]:
            continue
        for e in range(4 * u, 4 * u + 4):
            v = to[e]
            nd = d + weight[e]
            if nd < dist[v]:
                dist[v] = nd
                heapq.heappush(heap, (nd, v))
    reached = [d for d in dist if d < inf]
    return f"{len(reached)}:{sum(reached)}:{max(reached)}"


def edge_list(n: int) -> list[tuple[int, int, int]]:
    rng = XorShift32()
    edges = []
    for _ in range(4 * n):
        u = rng.next() % n
        v = rng.next() % n
        edges.append((rng.next() % 1000000, u, v))
    return edges


def find(parent: list[int], x: int) -> int:
    root = x
    while parent[root] != root:
        root = parent[root]
    while parent[x] != root:
        parent[x], x = root, parent[x]
    return root


def kruskal(n: int, edges: list[tuple[int, int, int]]) -> str:
    edges.sort(key=itemgetter(0))
    parent = list(range(n))
    rank = [0] * n
    count = total = 0
    for w, u, v in edges:
        ru = find(parent, u)
        rv = find(parent, v)
        if ru == rv:
            continue
        if rank[ru] < rank[rv]:
            ru, rv = rv, ru
        parent[rv] = ru
        if rank[ru] == rank[rv]:
            rank[ru] += 1
        count += 1
        total += w
    return f"{count}:{total}"


def unite(parent: list[int], rank: list[int], u: int, v: int) -> bool:
    ru = find(parent, u)
    rv = find(parent, v)
    if ru == rv:
        return False
    if rank[ru] < rank[rv]:
        ru, rv = rv, ru
    parent[rv] = ru
    if rank[ru] == rank[rv]:
        rank[ru] += 1
    return True


def union_all(n: int, edges: list[tuple[int, int, int]]) -> tuple[int, int]:
    parent = list(range(n))
    rank = [0] * n
    count = total = 0
    for w, u, v in edges:
        if unite(parent, rank, u, v):
            count += 1
            total += w
    return count, total


def kruskal_split(n: int, edges: list[tuple[int, int, int]]) -> tuple[str, dict]:
    """kruskal with the sort and the union-find timed apart (extra: sort_ms, union_find_ms)."""
    t0 = time.perf_counter_ns()
    edges.sort(key=itemgetter(0))
    t1 = time.perf_counter_ns()
    count, total = union_all(n, edges)
    t2 = time.perf_counter_ns()
    return f"{count}:{total}", {"sort_ms": round((t1 - t0) / 1e6, 3), "union_find_ms": round((t2 - t1) / 1e6, 3)}


def timed_edge_list(n: int) -> tuple[int, list[tuple[int, int, int]], float]:
    """kruskal-split's input, built anew before every repeat (untimed by the harness) with its own build timer."""
    t0 = time.perf_counter_ns()
    edges = edge_list(n)
    return n, edges, round((time.perf_counter_ns() - t0) / 1e6, 3)


def kruskal_split_built(n: int, edges: list[tuple[int, int, int]], build_ms: float) -> tuple[str, dict]:
    """kruskal_split plus build_ms, the time timed_edge_list took to make this repeat's edges."""
    checksum, extra = kruskal_split(n, edges)
    return checksum, {"build_ms": build_ms, **extra}


PACK_BITS = 21
PACK_MASK = (1 << PACK_BITS) - 1


def packed_edges(edges: list[tuple[int, int, int]]) -> list[int]:
    """One int per edge, w << 42 | u << 21 | v (n < 2^21), so the sort compares plain ints."""
    return [(w << 2 * PACK_BITS) | (u << PACK_BITS) | v for w, u, v in edges]


def kruskal_packed(n: int, keys: list[int]) -> tuple[str, dict]:
    t0 = time.perf_counter_ns()
    keys.sort()
    t1 = time.perf_counter_ns()
    parent = list(range(n))
    rank = [0] * n
    count = total = 0
    for k in keys:
        if unite(parent, rank, (k >> PACK_BITS) & PACK_MASK, k & PACK_MASK):
            count += 1
            total += k >> 2 * PACK_BITS
    t2 = time.perf_counter_ns()
    return f"{count}:{total}", {"sort_ms": round((t1 - t0) / 1e6, 3), "union_find_ms": round((t2 - t1) / 1e6, 3)}


def timed_packed_edges(g: tuple[int, list[tuple[int, int, int]]]) -> tuple[int, list[int], float]:
    """kruskal-packed's input, packed anew from the built edges before every repeat (untimed by the harness), with
    its own pack timer."""
    n, edges = g
    t0 = time.perf_counter_ns()
    keys = packed_edges(edges)
    return n, keys, round((time.perf_counter_ns() - t0) / 1e6, 3)


def kruskal_packed_timed(n: int, keys: list[int], pack_ms: float) -> tuple[str, dict]:
    """kruskal_packed plus pack_ms, the time timed_packed_edges took to pack this repeat's keys."""
    checksum, extra = kruskal_packed(n, keys)
    return checksum, {"pack_ms": pack_ms, **extra}


def weight_sort(weights: list[int]) -> str:
    weights.sort()
    return f"{len(weights)}:{h31(weights)}"


class Node:
    __slots__ = ("count", "key", "left", "right")

    def __init__(self, key: int) -> None:
        self.key = key
        self.count = 1
        self.left: Node | None = None
        self.right: Node | None = None


def bst(inserts: list[int], lookups: list[int]) -> str:
    root: Node | None = None
    node: Node | None
    size = 0
    for k in inserts:
        if root is None:
            root = Node(k)
            size = 1
            continue
        node = root
        while True:
            if k < node.key:
                if node.left is None:
                    node.left = Node(k)
                    size += 1
                    break
                node = node.left
            elif k > node.key:
                if node.right is None:
                    node.right = Node(k)
                    size += 1
                    break
                node = node.right
            else:
                node.count += 1
                break
    hits = 0
    for k in lookups:
        node = root
        while node is not None and node.key != k:
            node = node.left if k < node.key else node.right
        if node is not None:
            hits += 1
    return f"{size}:{hits}:{h31(in_order(root))}"


def in_order(root: Node | None) -> list[int]:
    keys = []
    stack: list[Node] = []
    node = root
    while stack or node is not None:
        while node is not None:
            stack.append(node)
            node = node.left
        node = stack.pop()
        keys.append(node.key)
        node = node.right
    return keys


def words(n: int) -> tuple[list[str], list[str]]:
    rng = XorShift32()

    def word(min_len: int, spread: int) -> str:
        return "".join(chr(97 + rng.next() % 26) for _ in range(min_len + rng.next() % spread))

    inserted = [word(3, 8) for _ in range(n)]
    prefixes = [word(1, 4) for _ in range(n)]
    return inserted, prefixes


class TrieNode:
    __slots__ = ("children", "count")

    def __init__(self) -> None:
        self.count = 0
        self.children: list[TrieNode | None] = [None] * 26


def trie(inserted: list[str], prefixes: list[str]) -> str:
    root = TrieNode()
    node: TrieNode | None
    nodes = 0
    for w in inserted:
        node = root
        for ch in w:
            i = ord(ch) - 97
            child = node.children[i]
            if child is None:
                child = node.children[i] = TrieNode()
                nodes += 1
            child.count += 1
            node = child
    total = 0
    for p in prefixes:
        node = root
        for ch in p:
            node = node.children[ord(ch) - 97]
            if node is None:
                break
        if node is not None:
            total += node.count
    return f"{nodes}:{total}"


def lru_keys(n: int) -> tuple[int, list[int]]:
    capacity = max(1, n // 100)
    return capacity, [k % (2 * capacity) for k in XorShift32().ints(n)]


def lru(capacity: int, keys: list[int]) -> str:
    cache: OrderedDict[int, int] = OrderedDict()
    hits = 0
    for i, k in enumerate(keys):
        if k in cache:
            cache.move_to_end(k)
            hits += 1
        else:
            if len(cache) == capacity:
                cache.popitem(last=False)
            cache[k] = i
    return f"{hits}:{len(keys) - hits}:{h31(cache)}"


def dna(length: int, count: int) -> list[str]:
    rng = XorShift32()
    return ["".join("".join([ACGT[v % 4] for v in part]) for part in chunks(rng, length)) for _ in range(count)]


def edit_distance(a: str, b: str) -> str:
    prev = list(range(len(b) + 1))
    cur = [0] * (len(b) + 1)
    for i, ca in enumerate(a, 1):
        cur[0] = i
        for j, cb in enumerate(b, 1):
            cost = 0 if ca == cb else 1
            cur[j] = min(prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost)
        prev, cur = cur, prev
    return str(prev[len(b)])


def items() -> list[tuple[int, int]]:
    rng = XorShift32()
    out = []
    for _ in range(KNAPSACK_ITEMS):
        weight = 1 + rng.next() % 1000
        out.append((weight, 1 + rng.next() % 1000))
    return out


def knapsack(capacity: int, things: list[tuple[int, int]]) -> str:
    best = [0] * (capacity + 1)
    for weight, value in things:
        for c in range(capacity, weight - 1, -1):
            if best[c - weight] + value > best[c]:
                best[c] = best[c - weight] + value
    return f"{best[capacity]}:{h31(best)}"


def lis(values: list[int]) -> str:
    tails: list[int] = []
    for x in values:
        i = bisect_left(tails, x)
        if i == len(tails):
            tails.append(x)
        else:
            tails[i] = x
    return f"{len(tails)}:{h31(tails)}"


def text(n: int) -> tuple[str, str]:
    t = dna(n, 1)[0]
    return t, t[n // 2 : n // 2 + PATTERN_LEN]


def english(n: int) -> str:
    """n bytes of English-like text: words from WORDS, separated by " ", ", " or ". " (SPEC.md section 12)."""
    rng = XorShift32()
    parts: list[str] = []
    size = 0
    while size < n:
        a = rng.next() % 64
        b = rng.next() % 64
        word = WORDS[min(a, b)]
        r = rng.next() % 16
        sep = ". " if r == 0 else ", " if r == 1 else " "
        parts.append(word)
        parts.append(sep)
        size += len(word) + len(sep)
    return "".join(parts)[:n]


def text_en(n: int) -> tuple[str, str]:
    t = english(n)
    return t, t[n // 2 : n // 2 + PATTERN_LEN]


def kmp(t: str, pattern: str) -> tuple[int, int, int]:
    m = len(pattern)
    fail = [0] * m
    k = 0
    for i in range(1, m):
        while k > 0 and pattern[i] != pattern[k]:
            k = fail[k - 1]
        if pattern[i] == pattern[k]:
            k += 1
        fail[i] = k
    count, first, last = 0, -1, -1
    k = 0
    for i, ch in enumerate(t):
        while k > 0 and ch != pattern[k]:
            k = fail[k - 1]
        if ch == pattern[k]:
            k += 1
        if k == m:
            last = i - m + 1
            if first < 0:
                first = last
            count += 1
            k = fail[k - 1]
    return count, first, last


def str_find(t: str, pattern: str) -> tuple[int, int, int]:
    count, first, last = 0, -1, -1
    pos = t.find(pattern)
    while pos >= 0:
        if first < 0:
            first = pos
        last = pos
        count += 1
        pos = t.find(pattern, pos + 1)
    return count, first, last


def matches(result: tuple[int, int, int]) -> str:
    return "{}:{}:{}".format(*result)


def fib(n: int) -> int:
    return n if n < 2 else fib(n - 1) + fib(n - 2)


def nqueens(n: int) -> str:
    cols = [False] * n
    up = [False] * (2 * n - 1)
    down = [False] * (2 * n - 1)

    def place(row: int) -> int:
        if row == n:
            return 1
        count = 0
        for col in range(n):
            if not cols[col] and not up[row + col] and not down[row - col + n - 1]:
                cols[col] = up[row + col] = down[row - col + n - 1] = True
                count += place(row + 1)
                cols[col] = up[row + col] = down[row - col + n - 1] = False
        return count

    return str(place(0))


def sorted_array(n: int) -> tuple[list[int], list[int]]:
    rng = XorShift32()
    a = [0] * n
    v = 0
    for i in range(n):
        v += 1 + rng.next() % 4
        a[i] = v
    return a, [rng.next() % (v + 1) for _ in range(n)]


def binary_search(a: list[int], queries: list[int]) -> str:
    n = len(a)
    hits = total = 0
    for q in queries:
        lo, hi = 0, n
        while lo < hi:
            mid = (lo + hi) // 2
            if a[mid] < q:
                lo = mid + 1
            else:
                hi = mid
        if lo < n and a[lo] == q:
            hits += 1
        total += lo
    return f"{hits}:{total}"


def kernels() -> dict:
    return {
        "bfs-grid": {"prepare": lambda n, _d: (n, grid(n)), "run": lambda g, _t, _p: bfs_grid(*g), "check": str},
        "dijkstra": {"prepare": lambda n, _d: (n, *graph(n)), "run": lambda g, _t, _p: dijkstra(*g), "check": str},
        "kruskal": {
            "prepare": lambda n, _d: (n, edge_list(n)),
            "fresh": lambda g: (g[0], g[1].copy()),
            "run": lambda g, _t, _p: kruskal(*g),
            "check": str,
        },
        "kruskal-split": {
            "prepare": lambda n, _d: n,
            "fresh": timed_edge_list,
            "run": lambda g, _t, _p: kruskal_split_built(*g),
            "check": lambda r: r,
        },
        "kruskal-packed": {
            "prepare": lambda n, _d: (n, edge_list(n)),
            "fresh": timed_packed_edges,
            "run": lambda g, _t, _p: kruskal_packed_timed(*g),
            "check": lambda r: r,
        },
        "kruskal-wsort": {
            "prepare": lambda n, _d: [w for w, _u, _v in edge_list(n)],
            "fresh": lambda ws: ws.copy(),
            "run": lambda ws, _t, _p: weight_sort(ws),
            "check": str,
        },
        "bst": {"prepare": lambda n, _d: hashmap_input(n), "run": lambda kv, _t, _p: bst(*kv), "check": str},
        "trie": {"prepare": lambda n, _d: words(n), "run": lambda w, _t, _p: trie(*w), "check": str},
        "lru": {"prepare": lambda n, _d: lru_keys(n), "run": lambda c, _t, _p: lru(*c), "check": str},
        "edit-distance": {
            "prepare": lambda n, _d: dna(n, 2),
            "run": lambda ab, _t, _p: edit_distance(*ab),
            "check": str,
        },
        "knapsack": {"prepare": lambda n, _d: (n, items()), "run": lambda c, _t, _p: knapsack(*c), "check": str},
        "lis": {"prepare": lambda n, _d: XorShift32().ints(n), "run": lambda a, _t, _p: lis(a), "check": str},
        "kmp": {"prepare": lambda n, _d: text(n), "run": lambda tp, _t, _p: kmp(*tp), "check": matches},
        "str-find": {"prepare": lambda n, _d: text(n), "run": lambda tp, _t, _p: str_find(*tp), "check": matches},
        "kmp-en": {"prepare": lambda n, _d: text_en(n), "run": lambda tp, _t, _p: kmp(*tp), "check": matches},
        "str-find-en": {
            "prepare": lambda n, _d: text_en(n),
            "run": lambda tp, _t, _p: str_find(*tp),
            "check": matches,
        },
        "fib": {"run": lambda n, _t, _p: fib(n), "check": str},
        "nqueens": {"run": lambda n, _t, _p: nqueens(n), "check": str},
        "binary-search": {
            "prepare": lambda n, _d: sorted_array(n),
            "run": lambda aq, _t, _p: binary_search(*aq),
            "check": str,
        },
    }


def hashmap_input(n: int) -> tuple[list[int], list[int]]:
    inserts = XorShift32().ints(n)
    return inserts, [k ^ (i & 1) for i, k in enumerate(inserts)]
