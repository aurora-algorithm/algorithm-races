// Race: bst-arena (java)
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

}
