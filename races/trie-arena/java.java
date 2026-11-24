// Race: trie-arena (java)
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

}
