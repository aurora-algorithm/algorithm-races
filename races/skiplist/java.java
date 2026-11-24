// Race: skiplist (java)
// Extracted from suite/java/src/main/java/kernels/Puzzle.java — build and run via suite/run.py

package kernels;
import java.math.BigInteger;
import java.util.Arrays;
import java.util.Map;
import java.util.PriorityQueue;
import java.util.function.Function;

import java.util.Arrays;

final class Puzzle {

  private static final long MASK = 0xFFFFFFFFL;

  private static final int SKIP_MAX_LEVEL = 24;

  private static final long SKIP_LEVEL_SEED = 88675123L;

  static final class SkipNode {
    final long key;
    final SkipNode[] next;

    SkipNode(long key, int height) {
      this.key = key;
      this.next = new SkipNode[height];
    }
  }

  record SkipInput(long[] keys, long[] lookups) {}

  record SkipResult(SkipNode head, int n, long hits, long heights) {}

  static SkipInput skiplistInput(int n) {
    var keys = Gen.ints(n);
    var lookups = new long[n];
    for (int i = 0; i < n; i++) {
      lookups[i] = keys[i] ^ (i & 1);
    }
    return new SkipInput(keys, lookups);
  }

  static SkipResult skiplist(SkipInput in) {
    var rng = new Gen.XorShift32(SKIP_LEVEL_SEED);
    var head = new SkipNode(-1, SKIP_MAX_LEVEL);
    int level = 1;
    var update = new SkipNode[SKIP_MAX_LEVEL];
    Arrays.fill(update, head);
    long heights = 0;
    for (long key : in.keys()) {
      var x = head;
      for (int i = level - 1; i >= 0; i--) {
        while (x.next[i] != null && x.next[i].key < key) {
          x = x.next[i];
        }
        update[i] = x;
      }
      if (x.next[0] != null && x.next[0].key == key) {
        continue;
      }
      int height = 1;
      while (height < SKIP_MAX_LEVEL && (rng.next() & 1) != 0) {
        height++;
      }
      for (int i = level; i < height; i++) {
        update[i] = head;
      }
      level = Math.max(level, height);
      var node = new SkipNode(key, height);
      for (int i = 0; i < height; i++) {
        node.next[i] = update[i].next[i];
        update[i].next[i] = node;
      }
      heights += height;
    }
    long hits = 0;
    for (long key : in.lookups()) {
      var x = head;
      for (int i = level - 1; i >= 0; i--) {
        while (x.next[i] != null && x.next[i].key < key) {
          x = x.next[i];
        }
      }
      if (x.next[0] != null && x.next[0].key == key) {
        hits++;
      }
    }
    return new SkipResult(head, in.keys().length, hits, heights);
  }

  static String skiplistChecksum(SkipResult result) {
    long h = 0;
    for (var x = result.head().next[0]; x != null; x = x.next[0]) {
      h = (h * 31 + x.key) & MASK;
    }
    return result.n() + ":" + result.hits() + ":" + result.heights() + ":" + h;
  }

}
