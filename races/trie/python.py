# Race: trie (python)
# Extracted from suite/python/algo.py — build and run via suite/run.py

from gen import XorShift32, h31


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
