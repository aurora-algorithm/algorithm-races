package kernels;

import java.lang.management.ManagementFactory;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Map;
import java.util.PriorityQueue;

/** Graph kernels of the algo group: BFS on a grid, Dijkstra, Kruskal. */
final class AlgoGraphs {
  private AlgoGraphs() {}

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

  /** Four edges per node u at 4u..4u+3: one to u+1 (a ring), three random. */
  record Graph(int n, int[] to, long[] weight) {}

  static Graph graph(int n) {
    var rng = new Gen.XorShift32();
    var to = new int[4 * n];
    var weight = new long[4 * n];
    for (int u = 0; u < n; u++) {
      to[4 * u] = (u + 1) % n;
      weight[4 * u] = 1 + rng.next() % 100;
      for (int e = 4 * u + 1; e < 4 * u + 4; e++) {
        to[e] = (int) (rng.next() % n);
        weight[e] = 1 + rng.next() % 100;
      }
    }
    return new Graph(n, to, weight);
  }

  record Entry(long dist, int node) {}

  /** A dijkstra result with the bytes this thread allocated during the run (-1: not reported). */
  record Counted(String checksum, long allocatedBytes) {
    Map<String, Long> extra() {
      return Map.of("allocated_bytes", allocatedBytes);
    }
  }

  private static final String HEAP_VARIANT =
      System.getenv().getOrDefault("KERNELS_DIJKSTRA_HEAP", "");

  private static long allocatedBytes() {
    try {
      if (ManagementFactory.getThreadMXBean() instanceof com.sun.management.ThreadMXBean bean) {
        return bean.getCurrentThreadAllocatedBytes();
      }
    } catch (UnsupportedOperationException e) {
      return -1;
    }
    return -1;
  }

  /**
   * dijkstra (PriorityQueue of records) or, with KERNELS_DIJKSTRA_HEAP=primitive,
   * dijkstraPrimitive, with the thread's allocated bytes around the call.
   */
  static Counted dijkstraCounted(Graph g) {
    boolean primitive = HEAP_VARIANT.equals("primitive");
    long before = allocatedBytes();
    String checksum = primitive ? dijkstraPrimitive(g) : dijkstra(g);
    long after = allocatedBytes();
    long bytes = before < 0 || after < 0 ? -1 : after - before;
    return new Counted(checksum, bytes);
  }

  /** The same search with a binary heap in two parallel primitive arrays (no object per push). */
  static String dijkstraPrimitive(Graph g) {
    int n = g.n();
    var dist = new long[n];
    Arrays.fill(dist, Long.MAX_VALUE);
    dist[0] = 0;
    long[] keys = new long[16];
    int[] nodes = new int[16];
    int size = 1;
    while (size > 0) {
      long d = keys[0];
      int u = nodes[0];
      size--;
      if (size > 0) {
        long lastKey = keys[size];
        int lastNode = nodes[size];
        int i = 0;
        while (true) {
          int c = 2 * i + 1;
          if (c >= size) {
            break;
          }
          if (c + 1 < size && keys[c + 1] < keys[c]) {
            c++;
          }
          if (keys[c] >= lastKey) {
            break;
          }
          keys[i] = keys[c];
          nodes[i] = nodes[c];
          i = c;
        }
        keys[i] = lastKey;
        nodes[i] = lastNode;
      }
      if (d > dist[u]) {
        continue;
      }
      for (int e = 4 * u; e < 4 * u + 4; e++) {
        int v = g.to()[e];
        long nd = d + g.weight()[e];
        if (nd < dist[v]) {
          dist[v] = nd;
          if (size == keys.length) {
            keys = Arrays.copyOf(keys, 2 * size);
            nodes = Arrays.copyOf(nodes, 2 * size);
          }
          int i = size++;
          while (i > 0) {
            int p = (i - 1) / 2;
            if (keys[p] <= nd) {
              break;
            }
            keys[i] = keys[p];
            nodes[i] = nodes[p];
            i = p;
          }
          keys[i] = nd;
          nodes[i] = v;
        }
      }
    }
    long reached = 0;
    long total = 0;
    long longest = 0;
    for (long dd : dist) {
      if (dd < Long.MAX_VALUE) {
        reached++;
        total += dd;
        longest = Math.max(longest, dd);
      }
    }
    return reached + ":" + total + ":" + longest;
  }

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
