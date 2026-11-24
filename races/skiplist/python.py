# Race: skiplist (python)
# Extracted from suite/python/puzzle.py — build and run via suite/run.py

from gen import MASK, XorShift32, fnv1a, h31


SKIP_MAX_LEVEL = 24


SKIP_LEVEL_SEED = 88675123


class SkipNode:
    __slots__ = ("key", "next")

    def __init__(self, key: int, height: int) -> None:
        self.key = key
        self.next: list[SkipNode | None] = [None] * height


def skiplist(keys: list[int], lookups: list[int]) -> tuple[SkipNode, int, int, int]:
    rng = XorShift32(SKIP_LEVEL_SEED)
    head = SkipNode(-1, SKIP_MAX_LEVEL)
    level = 1
    update: list[SkipNode] = [head] * SKIP_MAX_LEVEL
    heights = 0
    for key in keys:
        x = head
        for i in range(level - 1, -1, -1):
            nxt = x.next[i]
            while nxt is not None and nxt.key < key:
                x = nxt
                nxt = x.next[i]
            update[i] = x
        nxt = x.next[0]
        if nxt is not None and nxt.key == key:
            continue
        height = 1
        while height < SKIP_MAX_LEVEL and rng.next() & 1:
            height += 1
        for i in range(level, height):
            update[i] = head
        level = max(level, height)
        node = SkipNode(key, height)
        for i in range(height):
            node.next[i] = update[i].next[i]
            update[i].next[i] = node
        heights += height
    hits = 0
    for key in lookups:
        x = head
        for i in range(level - 1, -1, -1):
            nxt = x.next[i]
            while nxt is not None and nxt.key < key:
                x = nxt
                nxt = x.next[i]
        nxt = x.next[0]
        if nxt is not None and nxt.key == key:
            hits += 1
    return head, len(keys), hits, heights
