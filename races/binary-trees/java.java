// Race: binary-trees (java)
// Extracted from suite/java/src/main/java/kernels/BenchmarksGame.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;
import java.util.Locale;
import java.util.function.IntBinaryOperator;

import java.util.function.IntBinaryOperator;

final class BenchmarksGame {

  static final int MIN_DEPTH = 4;

  private BenchmarksGame() {}

  record Node(Node left, Node right) {}

  static Node makeTree(int depth) {
    if (depth == 0) {
      return new Node(null, null);
    }
    return new Node(makeTree(depth - 1), makeTree(depth - 1));
  }

  static int checkTree(Node node) {
    if (node.left() == null) {
      return 1;
    }
    return 1 + checkTree(node.left()) + checkTree(node.right());
  }

  static int depthChecks(int depth, int iterations) {
    int check = 0;
    for (int i = 0; i < iterations; i++) {
      check += checkTree(makeTree(depth));
    }
    return check;
  }

  /** The output text; {@code depthRun} sums the checks of (depth, iterations) trees. */
  static String binaryTreesLines(int n, IntBinaryOperator depthRun) {
    int maxDepth = Math.max(MIN_DEPTH + 2, n);
    int stretch = maxDepth + 1;
    var out = new StringBuilder();
    out.append(
        "stretch tree of depth " + stretch + "\t check: " + checkTree(makeTree(stretch)) + "\n");
    var longLived = makeTree(maxDepth);
    for (int depth = MIN_DEPTH; depth <= maxDepth; depth += 2) {
      int iterations = 1 << (maxDepth - depth + MIN_DEPTH);
      int check = depthRun.applyAsInt(depth, iterations);
      out.append(iterations + "\t trees of depth " + depth + "\t check: " + check + "\n");
    }
    out.append("long lived tree of depth " + maxDepth + "\t check: " + checkTree(longLived) + "\n");
    return out.toString();
  }

  static String binaryTrees(int n) {
    return binaryTreesLines(n, BenchmarksGame::depthChecks);
  }

}
