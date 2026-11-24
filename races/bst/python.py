# Race: bst (python)
# Extracted from suite/python/algo.py — build and run via suite/run.py

from gen import XorShift32, h31


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


def hashmap_input(n: int) -> tuple[list[int], list[int]]:
    inserts = XorShift32().ints(n)
    return inserts, [k ^ (i & 1) for i, k in enumerate(inserts)]
