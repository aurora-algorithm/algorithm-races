package kernels;

import java.util.ArrayDeque;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.TreeMap;

/** Tree and cache kernels of the algo group: BST, TreeMap, trie, LRU cache. */
final class AlgoTrees {
  private AlgoTrees() {}

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

  static String orderedMap(Misc.HashmapInput in) {
    var counts = new TreeMap<Long, Long>();
    for (long k : in.inserts()) {
      counts.merge(k, 1L, Long::sum);
    }
    int hits = 0;
    for (long k : in.lookups()) {
      if (counts.containsKey(k)) {
        hits++;
      }
    }
    var keys = counts.keySet().stream().mapToLong(Long::longValue).toArray();
    return counts.size() + ":" + hits + ":" + Gen.h31(keys);
  }

  record Words(String[] inserted, String[] prefixes) {}

  private static String word(Gen.XorShift32 rng, int minLength, int spread) {
    var chars = new char[minLength + (int) (rng.next() % spread)];
    for (int c = 0; c < chars.length; c++) {
      chars[c] = (char) ('a' + rng.next() % 26);
    }
    return new String(chars);
  }

  static Words words(int n) {
    var rng = new Gen.XorShift32();
    var inserted = new String[n];
    for (int i = 0; i < n; i++) {
      inserted[i] = word(rng, 3, 8);
    }
    var prefixes = new String[n];
    for (int i = 0; i < n; i++) {
      prefixes[i] = word(rng, 1, 4);
    }
    return new Words(inserted, prefixes);
  }

  private static final class TrieNode {
    long count;
    final TrieNode[] children = new TrieNode[26];
  }

  static String trie(Words w) {
    var root = new TrieNode();
    long nodes = 0;
    for (var word : w.inserted()) {
      var node = root;
      for (int i = 0; i < word.length(); i++) {
        int c = word.charAt(i) - 'a';
        var child = node.children[c];
        if (child == null) {
          child = new TrieNode();
          node.children[c] = child;
          nodes++;
        }
        child.count++;
        node = child;
      }
    }
    long total = 0;
    for (var prefix : w.prefixes()) {
      var node = root;
      for (int i = 0; i < prefix.length() && node != null; i++) {
        node = node.children[prefix.charAt(i) - 'a'];
      }
      if (node != null) {
        total += node.count;
      }
    }
    return nodes + ":" + total;
  }

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
