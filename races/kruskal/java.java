// Race: kruskal (java)
// Extracted from suite/java/src/main/java/kernels/AlgoGraphs.java — build and run via suite/run.py

package kernels;
import java.lang.management.ManagementFactory;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Map;
import java.util.PriorityQueue;

import java.util.Arrays;

import java.util.Comparator;

final class AlgoGraphs {

  record Edge(int w, int u, int v) {}

  record EdgeList(int n, Edge[] edges) {
    EdgeList copy() {
      return new EdgeList(n, edges.clone());
    }
  }

  static EdgeList edgeList(int n) {
    var rng = new Gen.XorShift32();
    var edges = new Edge[4 * n];
    for (int i = 0; i < edges.length; i++) {
      int u = (int) (rng.next() % n);
      int v = (int) (rng.next() % n);
      edges[i] = new Edge((int) (rng.next() % 1000000), u, v);
    }
    return new EdgeList(n, edges);
  }

  static int find(int[] parent, int x) {
    int root = x;
    while (parent[root] != root) {
      root = parent[root];
    }
    while (parent[x] != root) {
      int next = parent[x];
      parent[x] = root;
      x = next;
    }
    return root;
  }

  static String kruskal(EdgeList list) {
    int n = list.n();
    var edges = list.edges();
    Arrays.sort(edges, Comparator.comparingInt(Edge::w));
    var parent = new int[n];
    for (int i = 0; i < n; i++) {
      parent[i] = i;
    }
    var rank = new byte[n];
    long count = 0;
    long total = 0;
    for (var e : edges) {
      int ru = find(parent, e.u());
      int rv = find(parent, e.v());
      if (ru == rv) {
        continue;
      }
      if (rank[ru] < rank[rv]) {
        int swap = ru;
        ru = rv;
        rv = swap;
      }
      parent[rv] = ru;
      if (rank[ru] == rank[rv]) {
        rank[ru]++;
      }
      count++;
      total += e.w();
    }
    return count + ":" + total;
  }

}
