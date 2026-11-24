// Race: bst (java)
// Extracted from suite/java/src/main/java/kernels/AlgoTrees.java — build and run via suite/run.py

package kernels;
import java.util.ArrayDeque;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.TreeMap;

import java.util.ArrayDeque;

final class AlgoTrees {

  private static final class Node {
    final long key;
    int count = 1;
    Node left;
    Node right;

    Node(long key) {
      this.key = key;
    }
  }

  static String bst(Misc.HashmapInput in) {
    Node root = null;
    int size = 0;
    for (long k : in.inserts()) {
      if (root == null) {
        root = new Node(k);
        size++;
        continue;
      }
      var node = root;
      while (true) {
        if (k < node.key) {
          if (node.left == null) {
            node.left = new Node(k);
            size++;
            break;
          }
          node = node.left;
        } else if (k > node.key) {
          if (node.right == null) {
            node.right = new Node(k);
            size++;
            break;
          }
          node = node.right;
        } else {
          node.count++;
          break;
        }
      }
    }
    int hits = 0;
    for (long k : in.lookups()) {
      var node = root;
      while (node != null && node.key != k) {
        node = k < node.key ? node.left : node.right;
      }
      if (node != null) {
        hits++;
      }
    }
    return size + ":" + hits + ":" + Gen.h31(inOrder(root, size));
  }

  private static long[] inOrder(Node root, int size) {
    var keys = new long[size];
    int i = 0;
    var stack = new ArrayDeque<Node>();
    var node = root;
    while (node != null || !stack.isEmpty()) {
      while (node != null) {
        stack.push(node);
        node = node.left;
      }
      node = stack.pop();
      keys[i++] = node.key;
      node = node.right;
    }
    return keys;
  }

}
