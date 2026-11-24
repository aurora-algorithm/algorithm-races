// Race: dijkstra (java)
// Extracted from suite/java/src/main/java/kernels/AlgoGraphs.java — build and run via suite/run.py

package kernels;
import java.lang.management.ManagementFactory;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Map;
import java.util.PriorityQueue;

import java.util.Arrays;

import java.util.Comparator;

import java.util.PriorityQueue;

final class AlgoGraphs {

  /** Four edges per node u at 4u..4u+3: one to u+1 (a ring), three random. */
  record Graph(int n, int[] to, long[] weight) {}

  record Entry(long dist, int node) {}

  private static final String HEAP_VARIANT =
      System.getenv().getOrDefault("KERNELS_DIJKSTRA_HEAP", "");

  static String dijkstra(Graph g) {
    int n = g.n();
    var dist = new long[n];
    Arrays.fill(dist, Long.MAX_VALUE);
    dist[0] = 0;
    var queue = new PriorityQueue<Entry>(Comparator.comparingLong(Entry::dist));
    queue.add(new Entry(0, 0));
    while (!queue.isEmpty()) {
      var top = queue.poll();
      int u = top.node();
      if (top.dist() > dist[u]) {
        continue;
      }
      for (int e = 4 * u; e < 4 * u + 4; e++) {
        int v = g.to()[e];
        long nd = top.dist() + g.weight()[e];
        if (nd < dist[v]) {
          dist[v] = nd;
          queue.add(new Entry(nd, v));
        }
      }
    }
    long reached = 0;
    long total = 0;
    long longest = 0;
    for (long d : dist) {
      if (d < Long.MAX_VALUE) {
        reached++;
        total += d;
        longest = Math.max(longest, d);
      }
    }
    return reached + ":" + total + ":" + longest;
  }

}
