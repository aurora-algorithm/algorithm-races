// Race: binary-trees-arena (java)
// Extracted from suite/java/src/main/java/kernels/Arena.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;
import java.util.Map;

import java.util.Arrays;

final class Arena {

  private static final int FIRST_CAPACITY = 16;

  /** The "no child" link of the BST and the binary trees. */
  private static final int NIL = -1;

  /**
   * One binary tree: node i's links at 2i (left) and 2i + 1 (right), children pushed before their
   * parent; a leaf's links are NIL.
   */
  private static final class TreeArena {
    int[] links = new int[2 * FIRST_CAPACITY];
    int count;

    int make(int depth) {
      int left = NIL;
      int right = NIL;
      if (depth > 0) {
        left = make(depth - 1);
        right = make(depth - 1);
      }
      if (2 * count == links.length) {
        links = Arrays.copyOf(links, 2 * links.length);
      }
      links[2 * count] = left;
      links[2 * count + 1] = right;
      return count++;
    }

    int check(int node) {
      int left = links[2 * node];
      return left == NIL ? 1 : 1 + check(left) + check(links[2 * node + 1]);
    }
  }

  private static int treeCheck(int depth) {
    var tree = new TreeArena();
    return tree.check(tree.make(depth));
  }

  static String binaryTrees(int n) {
    int minDepth = BenchmarksGame.MIN_DEPTH;
    int maxDepth = Math.max(minDepth + 2, n);
    int stretch = maxDepth + 1;
    var out = new StringBuilder();
    out.append("stretch tree of depth " + stretch + "\t check: " + treeCheck(stretch) + "\n");
    var longLived = new TreeArena();
    int longRoot = longLived.make(maxDepth);
    for (int depth = minDepth; depth <= maxDepth; depth += 2) {
      int iterations = 1 << (maxDepth - depth + minDepth);
      int check = 0;
      for (int i = 0; i < iterations; i++) {
        check += treeCheck(depth);
      }
      out.append(iterations + "\t trees of depth " + depth + "\t check: " + check + "\n");
    }
    out.append(
        "long lived tree of depth " + maxDepth + "\t check: " + longLived.check(longRoot) + "\n");
    return out.toString();
  }

}
