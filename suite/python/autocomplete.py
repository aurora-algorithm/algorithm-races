"""Autocomplete kernels (../SPEC.md section 15): the first 10 words that start with a typed prefix, from a sorted list
with binary search, a 26-slot trie, or the same trie in one arena array (no Python row: see SPEC)."""

import bisect
from pathlib import Path

from gen import MASK, XorShift32

WORDS_FILE = "words-web2.txt"
TOP_K = 10


def h31(h: int, v: int) -> int:
    return (h * 31 + v) & MASK


def shuffled_words(data: Path) -> tuple[list[str], XorShift32]:
    words = (data / WORDS_FILE).read_text(encoding="ascii").split()
    rng = XorShift32()
    for i in range(len(words) - 1, 0, -1):
        j = rng.next() % (i + 1)
        words[i], words[j] = words[j], words[i]
    return words, rng


def build_input(n: int, data: Path) -> list[str]:
    words, _ = shuffled_words(data)
    return words[:n]


def query_input(n: int, data: Path) -> tuple[list[str], list[str]]:
    words, rng = shuffled_words(data)
    queries = []
    for _ in range(n):
        w = words[rng.next() % len(words)]
        queries.append(w[: min(2 + rng.next() % 4, len(w))])
    return words, queries


def fold(h: int, w: str) -> int:
    return h31(h31(h, len(w)), ord(w[-1]))


def sorted_build(words: list[str]) -> list[str]:
    a = words.copy()
    a.sort()
    return a


def sorted_complete(a: list[str], prefix: str, out: list[str]) -> None:
    i = bisect.bisect_left(a, prefix)
    end = min(i + TOP_K, len(a))
    while i < end and a[i].startswith(prefix):
        out.append(a[i])
        i += 1


class TrieNode:
    __slots__ = ("children", "word")

    def __init__(self) -> None:
        self.children: list[TrieNode | None] = [None] * 26
        self.word = -1


def trie_build(words: list[str]) -> tuple[TrieNode, int]:
    root = TrieNode()
    nodes = 1
    for idx, w in enumerate(words):
        node = root
        for ch in w:
            i = ord(ch) - 97
            child = node.children[i]
            if child is None:
                child = node.children[i] = TrieNode()
                nodes += 1
            node = child
        node.word = idx
    return root, nodes


def collect(node: TrieNode, words: list[str], out: list[str], limit: int) -> None:
    if node.word >= 0:
        out.append(words[node.word])
    for child in node.children:
        if len(out) >= limit:
            return
        if child is not None:
            collect(child, words, out, limit)


def trie_complete(root: TrieNode, words: list[str], prefix: str, out: list[str]) -> None:
    node: TrieNode | None = root
    for ch in prefix:
        node = node.children[ord(ch) - 97]
        if node is None:
            return
    collect(node, words, out, TOP_K)


def run_queries(complete, queries: list[str]) -> tuple[int, int]:
    results = 0
    h = 0
    out: list[str] = []
    for q in queries:
        out.clear()
        complete(q, out)
        results += len(out)
        for w in out:
            h = fold(h, w)
    return results, h


def build_check(in_order: list[str]) -> str:
    h = 0
    for w in in_order:
        h = fold(h, w)
    return f"{len(in_order)}:{h}"


def trie_build_check(built: tuple[tuple[TrieNode, int], list[str]]) -> tuple[str, dict]:
    (root, nodes), words = built
    out: list[str] = []
    collect(root, words, out, len(words) + 1)
    return build_check(out), {"nodes": nodes}


def drop_last(built: dict, words: list[str]) -> list[str]:
    built.clear()
    return words


def keep(built: dict, value):
    built["last"] = value
    return None


def kernels() -> dict:
    def sorted_query(wq):
        a = sorted_build(wq[0])
        return lambda: run_queries(lambda q, out: sorted_complete(a, q, out), wq[1]), len(wq[1])

    def trie_query(wq):
        root, _ = trie_build(wq[0])
        return lambda: run_queries(lambda q, out: trie_complete(root, wq[0], q, out), wq[1]), len(wq[1])

    def query_check(r: tuple[tuple[int, int], int]) -> str:
        (results, h), n = r
        return f"{n}:{results}:{h}"

    # The last trie lives here, not in main's `result`, so `fresh` frees it before the next timed build: a live
    # 759k-node trie from the previous repeat makes CPython's cyclic GC scan it during the next one.
    built: dict = {}

    return {
        "ac-sorted-build": {
            "prepare": build_input,
            "run": lambda w, _t, _p: sorted_build(w),
            "check": build_check,
        },
        "ac-trie-build": {
            "prepare": build_input,
            "fresh": lambda w: drop_last(built, w),
            "run": lambda w, _t, _p: keep(built, (trie_build(w), w)),
            "check": lambda _r: trie_build_check(built["last"]),
        },
        "ac-sorted": {
            "prepare": lambda n, d: sorted_query(query_input(n, d)),
            "run": lambda qn, _t, _p: (qn[0](), qn[1]),
            "check": query_check,
        },
        "ac-trie": {
            "prepare": lambda n, d: trie_query(query_input(n, d)),
            "run": lambda qn, _t, _p: (qn[0](), qn[1]),
            "check": query_check,
        },
    }
