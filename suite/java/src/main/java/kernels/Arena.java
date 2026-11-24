package kernels;

import java.util.Arrays;
import java.util.Map;

/**
 * The -arena kernels (kernels/SPEC.md section 12, "Arena rows"): bst, trie and binary-trees with
 * every node in growable int arrays and 32-bit index links, the same algorithms and checksums as
 * the object versions.
 */
final class Arena {
  private static final int FIRST_CAPACITY = 16;

  /** The "no child" link of the BST and the binary trees. */
  private static final int NIL = -1;

  private Arena() {}

  static Map<String, Main.Kernel<?, ?>> kernels() {
    return Map.ofEntries(
        Map.entry(
            "bst-arena",
            new Main.Kernel<>(
                (n, data) -> Misc.hashmapInput(n), m -> m, (m, ctx) -> bst(m), s -> s)),
        Map.entry(
            "trie-arena",
            new Main.Kernel<>(
                (n, data) -> AlgoTrees.words(n), w -> w, (w, ctx) -> trie(w), s -> s)),
        Map.entry("binary-trees-arena", Main.Kernel.sized((n, ctx) -> binaryTrees(n), Gen::fnv1a)));
  }

  /**
   * One BST node per index in four parallel arrays, grown together. push may replace the arrays, so
   * a caller reads t.left or t.right only after it returns.
   */
  private static final class BstNodes {
    long[] key = new long[FIRST_CAPACITY];
    int[] count = new int[FIRST_CAPACITY];
    int[] left = new int[FIRST_CAPACITY];
    int[] right = new int[FIRST_CAPACITY];
    int size;

    int push(long k) {
      if (size == key.length) {
        key = Arrays.copyOf(key, 2 * size);
        count = Arrays.copyOf(count, 2 * size);
        left = Arrays.copyOf(left, 2 * size);
        right = Arrays.copyOf(right, 2 * size);
      }
      key[size] = k;
      count[size] = 1;
      left[size] = NIL;
      right[size] = NIL;
      return size++;
    }
  }

  static String bst(Misc.HashmapInput in) {
    var t = new BstNodes();
    for (long k : in.inserts()) {
      if (t.size == 0) {
        t.push(k);
        continue;
      }
      int cur = 0;
      while (true) {
        if (k < t.key[cur]) {
          if (t.left[cur] == NIL) {
            int fresh = t.push(k);
            t.left[cur] = fresh;
            break;
          }
          cur = t.left[cur];
        } else if (k > t.key[cur]) {
          if (t.right[cur] == NIL) {
            int fresh = t.push(k);
            t.right[cur] = fresh;
            break;
          }
          cur = t.right[cur];
        } else {
          t.count[cur]++;
          break;
        }
      }
    }
    int hits = 0;
    for (long k : in.lookups()) {
      int cur = t.size == 0 ? NIL : 0;
      while (cur != NIL && t.key[cur] != k) {
        cur = k < t.key[cur] ? t.left[cur] : t.right[cur];
      }
      if (cur != NIL) {
        hits++;
      }
    }
    return t.size + ":" + hits + ":" + Gen.h31(inOrder(t));
  }

  private static long[] inOrder(BstNodes t) {
    var keys = new long[t.size];
    int i = 0;
    var stack = new int[FIRST_CAPACITY];
    int depth = 0;
    int cur = t.size == 0 ? NIL : 0;
    while (cur != NIL || depth > 0) {
      while (cur != NIL) {
        if (depth == stack.length) {
          stack = Arrays.copyOf(stack, 2 * depth);
        }
        stack[depth++] = cur;
        cur = t.left[cur];
      }
      cur = stack[--depth];
      keys[i++] = t.key[cur];
      cur = t.right[cur];
    }
    return keys;
  }

  /**
   * A trie node is 27 ints at node * 27: the count, then 26 child links. Node 0 is the root and
   * never anyone's child, so a child link of 0 means no child.
   */
  private static final int TRIE_STRIDE = 27;

  static String trie(AlgoTrees.Words w) {
    var nodes = new int[FIRST_CAPACITY * TRIE_STRIDE];
    int count = 1;
    for (var word : w.inserted()) {
      int node = 0;
      for (int i = 0; i < word.length(); i++) {
        int slot = node * TRIE_STRIDE + 1 + word.charAt(i) - 'a';
        if (nodes[slot] == 0) {
          if ((count + 1) * TRIE_STRIDE > nodes.length) {
            nodes = Arrays.copyOf(nodes, 2 * nodes.length);
          }
          nodes[slot] = count++;
        }
        node = nodes[slot];
        nodes[node * TRIE_STRIDE]++;
      }
    }
    long total = 0;
    for (var prefix : w.prefixes()) {
      // A prefix has at least one letter, so node 0 after the walk means it is absent.
      int node = 0;
      for (int i = 0; i < prefix.length(); i++) {
        node = nodes[node * TRIE_STRIDE + 1 + prefix.charAt(i) - 'a'];
        if (node == 0) {
          break;
        }
      }
      if (node != 0) {
        total += nodes[node * TRIE_STRIDE];
      }
    }
    return (count - 1) + ":" + total;
  }

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
