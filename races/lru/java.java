// Race: lru (java)
// Extracted from suite/java/src/main/java/kernels/AlgoTrees.java — build and run via suite/run.py

package kernels;
import java.util.ArrayDeque;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.TreeMap;

import java.util.LinkedHashMap;

import java.util.Map;

final class AlgoTrees {

  record LruKeys(int capacity, int[] keys) {}

  static LruKeys lruKeys(int n) {
    int capacity = Math.max(1, n / 100);
    var rng = new Gen.XorShift32();
    var keys = new int[n];
    for (int i = 0; i < n; i++) {
      keys[i] = (int) (rng.next() % (2L * capacity));
    }
    return new LruKeys(capacity, keys);
  }

  /**
   * LinkedHashMap in access order: get() moves a key to the newest end, put() evicts the eldest.
   */
  private static final class LruCache extends LinkedHashMap<Integer, Integer> {
    private static final long serialVersionUID = 1L;
    private final int capacity;

    LruCache(int capacity) {
      super(capacity * 4 / 3 + 1, 0.75f, true);
      this.capacity = capacity;
    }

    @Override
    protected boolean removeEldestEntry(Map.Entry<Integer, Integer> eldest) {
      return size() > capacity;
    }
  }

  static String lru(LruKeys in) {
    var cache = new LruCache(in.capacity());
    int hits = 0;
    var keys = in.keys();
    for (int i = 0; i < keys.length; i++) {
      if (cache.get(keys[i]) != null) {
        hits++;
      } else {
        cache.put(keys[i], i);
      }
    }
    var oldestFirst = cache.keySet().stream().mapToLong(Integer::longValue).toArray();
    return hits + ":" + (keys.length - hits) + ":" + Gen.h31(oldestFirst);
  }

}
