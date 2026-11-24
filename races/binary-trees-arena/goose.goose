// Race: binary-trees-arena (goose)
// Extracted from suite/goose/bg.goose — build and run via suite/run.py

import std;
import math;

let MIN_DEPTH = 4;

// A tree is built into the pool its caller owns, one push per node: Goose has no heap, so the pool is how a
// program allocates nodes, and dropping the pool at the end of its scope frees the whole tree at once.
struct TreeNode { left: TreeNode&<u32>?, right: TreeNode&<u32>? }

recursive fn make_tree(pool: TreeNode[>..]&, depth: i64) -> TreeNode& {
    if depth == 0 { return pool.push(TreeNode {}); }
    let left:  TreeNode& = make_tree(pool, depth - 1);
    let right: TreeNode& = make_tree(pool, depth - 1);
    return pool.push(TreeNode { left: left, right: right });
}

recursive fn check_tree(node: TreeNode&) -> i64 {
    let left = node.left;
    guard left else { return 1; }
    let right = node.right;
    guard right else { return 1; }
    return 1 + check_tree(left) + check_tree(right);
}

fn tree_check(depth: i64) -> i64 {
    var pool: TreeNode[>..] = [];
    return check_tree(make_tree(pool, depth));
}

fn binary_trees(n: i64, out: u8[>..]&) {
    let max_depth = max(MIN_DEPTH + 2, n);
    let stretch = max_depth + 1;
    format(out, "stretch tree of depth ", stretch, "\t check: ", tree_check(stretch), "\n");
    var long_pool: TreeNode[>..] = [];
    let long_lived: TreeNode& = make_tree(long_pool, max_depth);
    var depth = MIN_DEPTH;
    while depth <= max_depth {
        let iterations = 1 << (max_depth - depth + MIN_DEPTH);
        var check = 0;
        for i in iterations { check += tree_check(depth); }
        format(out, iterations, "\t trees of depth ", depth, "\t check: ", check, "\n");
        depth += 2;
    }
    format(out, "long lived tree of depth ", max_depth, "\t check: ", check_tree(long_lived), "\n");
}
