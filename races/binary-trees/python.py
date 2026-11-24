# Race: binary-trees (python)
# Extracted from suite/python/bg.py — build and run via suite/run.py

MIN_DEPTH = 4


def make_tree(depth: int):
    if depth == 0:
        return (None, None)
    depth -= 1
    return (make_tree(depth), make_tree(depth))


def check_tree(node) -> int:
    left, right = node
    if left is None:
        return 1
    return 1 + check_tree(left) + check_tree(right)


def depth_checks(depth: int, iterations: int) -> int:
    check = 0
    for _ in range(iterations):
        check += check_tree(make_tree(depth))
    return check


def binary_trees_lines(n: int, depth_run) -> str:
    max_depth = max(MIN_DEPTH + 2, n)
    stretch = max_depth + 1
    out = [f"stretch tree of depth {stretch}\t check: {check_tree(make_tree(stretch))}"]
    long_lived = make_tree(max_depth)
    for depth in range(MIN_DEPTH, max_depth + 1, 2):
        iterations = 1 << (max_depth - depth + MIN_DEPTH)
        out.append(f"{iterations}\t trees of depth {depth}\t check: {depth_run(depth, iterations)}")
    out.append(f"long lived tree of depth {max_depth}\t check: {check_tree(long_lived)}")
    return "\n".join(out) + "\n"


def binary_trees(n: int) -> str:
    return binary_trees_lines(n, depth_checks)
