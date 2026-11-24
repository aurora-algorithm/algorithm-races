// Race: trie (java)
// Extracted from suite/java/src/main/java/kernels/AlgoTrees.java — build and run via suite/run.py

package kernels;
import java.util.ArrayDeque;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.TreeMap;

final class AlgoTrees {

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

}
