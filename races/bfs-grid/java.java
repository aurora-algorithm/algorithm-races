// Race: bfs-grid (java)
// Extracted from suite/java/src/main/java/kernels/AlgoGraphs.java — build and run via suite/run.py

package kernels;
import java.lang.management.ManagementFactory;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Map;
import java.util.PriorityQueue;

import java.util.Arrays;

final class AlgoGraphs {

  record Grid(int n, boolean[] walls) {}

  static Grid grid(int n) {
    var rng = new Gen.XorShift32();
    var walls = new boolean[n * n];
    for (int i = 0; i < walls.length; i++) {
      walls[i] = rng.next() % 100 < 30;
    }
    for (int cell : new int[] {0, 1, n, n * n - n - 1, n * n - 2, n * n - 1}) {
      walls[cell] = false;
    }
    return new Grid(n, walls);
  }

  static String bfsGrid(Grid g) {
    int n = g.n();
    var walls = g.walls();
    var dist = new int[n * n];
    Arrays.fill(dist, -1);
    dist[0] = 0;
    // Each cell is queued at most once, so n*n ints with a head and a tail index hold the queue.
    var queue = new int[n * n];
    int tail = 0;
    queue[tail++] = 0;
    for (int head = 0; head < tail; head++) {
      int cell = queue[head];
      int d = dist[cell] + 1;
      int row = cell / n;
      int col = cell % n;
      tail = visit(cell - n, row > 0, d, walls, dist, queue, tail);
      tail = visit(cell + n, row < n - 1, d, walls, dist, queue, tail);
      tail = visit(cell - 1, col > 0, d, walls, dist, queue, tail);
      tail = visit(cell + 1, col < n - 1, d, walls, dist, queue, tail);
    }
    long reached = 0;
    long total = 0;
    for (int d : dist) {
      if (d >= 0) {
        reached++;
        total += d;
      }
    }
    return reached + ":" + dist[n * n - 1] + ":" + total;
  }

  private static int visit(
      int next, boolean ok, int d, boolean[] walls, int[] dist, int[] queue, int tail) {
    if (ok && !walls[next] && dist[next] < 0) {
      dist[next] = d;
      queue[tail++] = next;
    }
    return tail;
  }

}
