using System.Runtime.CompilerServices;

namespace Kernels;

/// <summary>Graph kernels of the algo group: BFS on a grid, Dijkstra, Kruskal.</summary>
public static class AlgoGraphs
{
    public readonly record struct Edge(uint W, int U, int V);

    public static bool[] Grid(int n)
    {
        var rng = new XorShift32();
        var walls = new bool[n * n];
        for (int i = 0; i < walls.Length; i++)
        {
            walls[i] = rng.Next() % 100 < 30;
        }
        foreach (int cell in new[] { 0, 1, n, n * n - n - 1, n * n - 2, n * n - 1 })
        {
            walls[cell] = false;
        }
        return walls;
    }

    public static string BfsGrid(int n, bool[] walls)
    {
        var dist = new int[n * n];
        Array.Fill(dist, -1);
        dist[0] = 0;
        var queue = new Queue<int>();
        queue.Enqueue(0);
        while (queue.TryDequeue(out int cell))
        {
            int d = dist[cell] + 1;
            int row = cell / n, col = cell % n;
            Visit(cell - n, row > 0);
            Visit(cell + n, row < n - 1);
            Visit(cell - 1, col > 0);
            Visit(cell + 1, col < n - 1);

            void Visit(int next, bool ok)
            {
                if (ok && !walls[next] && dist[next] < 0)
                {
                    dist[next] = d;
                    queue.Enqueue(next);
                }
            }
        }
        long reached = 0, total = 0;
        foreach (int d in dist)
        {
            if (d >= 0)
            {
                reached++;
                total += d;
            }
        }
        return Algo.Invariant($"{reached}:{dist[n * n - 1]}:{total}");
    }

    /// <summary>Four edges per node u at 4u..4u+3: one to u+1 (a ring, so every node is reachable), three random.</summary>
    public static (int[] To, long[] Weight) Graph(int n)
    {
        var rng = new XorShift32();
        var to = new int[4 * n];
        var weight = new long[4 * n];
        for (int u = 0; u < n; u++)
        {
            to[4 * u] = (u + 1) % n;
            weight[4 * u] = 1 + rng.Next() % 100;
            for (int e = 4 * u + 1; e < 4 * u + 4; e++)
            {
                to[e] = (int)(rng.Next() % (uint)n);
                weight[e] = 1 + rng.Next() % 100;
            }
        }
        return (to, weight);
    }

    /// <summary>
    /// The dijkstra kernel. Extra: bytes this thread allocated per run and gen-0 collections per run, read before and
    /// after the timed repeats (never inside them), and the size of the queue's (element, priority) entry. With
    /// KERNELS_DIJKSTRA_COUNTS=1, an untimed counting run afterwards adds pushes, pops and stale_pops.
    /// </summary>
    public static Measurement DijkstraKernel(Options o)
    {
        var graph = Graph(o.Size);
        long bytesBefore = GC.GetAllocatedBytesForCurrentThread();
        int gen0Before = GC.CollectionCount(0);
        Measurement m = KernelTable.Measure(o, () => graph, g => Dijkstra(o.Size, g), s => s);
        long bytesAfter = GC.GetAllocatedBytesForCurrentThread();
        int gen0After = GC.CollectionCount(0);
        var extra = new Dictionary<string, long>
        {
            ["alloc_bytes_per_run"] = (bytesAfter - bytesBefore) / o.Repeats,
            ["gen0_gcs_per_run_x1000"] = (gen0After - gen0Before) * 1000L / o.Repeats,
            ["entry_bytes"] = Unsafe.SizeOf<(int, long)>(),
        };
        if (Environment.GetEnvironmentVariable("KERNELS_DIJKSTRA_COUNTS") == "1")
        {
            var (pushes, pops, stale, checksum) = DijkstraCounts(o.Size, graph);
            if (checksum != m.Checksum)
            {
                throw new InvalidOperationException($"counting run checksum {checksum}, timed run {m.Checksum}");
            }
            extra["pushes"] = pushes;
            extra["pops"] = pops;
            extra["stale_pops"] = stale;
        }
        return m with { Extra = extra };
    }

    /// <summary>Dijkstra as in <see cref="Dijkstra"/>, counting pushes, pops and stale pops. Never timed.</summary>
    private static (long Pushes, long Pops, long Stale, string Checksum) DijkstraCounts(int n, (int[] To, long[] Weight) g)
    {
        var dist = new long[n];
        Array.Fill(dist, long.MaxValue);
        dist[0] = 0;
        var queue = new PriorityQueue<int, long>();
        queue.Enqueue(0, 0);
        long pushes = 1, pops = 0, stale = 0;
        while (queue.TryDequeue(out int u, out long d))
        {
            pops++;
            if (d > dist[u])
            {
                stale++;
                continue;
            }
            for (int e = 4 * u; e < 4 * u + 4; e++)
            {
                int v = g.To[e];
                long nd = d + g.Weight[e];
                if (nd < dist[v])
                {
                    dist[v] = nd;
                    pushes++;
                    queue.Enqueue(v, nd);
                }
            }
        }
        long reached = 0, total = 0, longest = 0;
        foreach (long d in dist)
        {
            if (d < long.MaxValue)
            {
                reached++;
                total += d;
                longest = Math.Max(longest, d);
            }
        }
        return (pushes, pops, stale, Algo.Invariant($"{reached}:{total}:{longest}"));
    }

    public static string Dijkstra(int n, (int[] To, long[] Weight) g)
    {
        var dist = new long[n];
        Array.Fill(dist, long.MaxValue);
        dist[0] = 0;
        var queue = new PriorityQueue<int, long>();
        queue.Enqueue(0, 0);
        while (queue.TryDequeue(out int u, out long d))
        {
            if (d > dist[u])
            {
                continue;
            }
            for (int e = 4 * u; e < 4 * u + 4; e++)
            {
                int v = g.To[e];
                long nd = d + g.Weight[e];
                if (nd < dist[v])
                {
                    dist[v] = nd;
                    queue.Enqueue(v, nd);
                }
            }
        }
        long reached = 0, total = 0, longest = 0;
        foreach (long d in dist)
        {
            if (d < long.MaxValue)
            {
                reached++;
                total += d;
                longest = Math.Max(longest, d);
            }
        }
        return Algo.Invariant($"{reached}:{total}:{longest}");
    }

    public static Edge[] EdgeList(int n)
    {
        var rng = new XorShift32();
        var edges = new Edge[4 * n];
        for (int i = 0; i < edges.Length; i++)
        {
            int u = (int)(rng.Next() % (uint)n);
            int v = (int)(rng.Next() % (uint)n);
            edges[i] = new Edge(rng.Next() % 1000000, u, v);
        }
        return edges;
    }

    private static int Find(int[] parent, int x)
    {
        int root = x;
        while (parent[root] != root)
        {
            root = parent[root];
        }
        while (parent[x] != root)
        {
            (parent[x], x) = (root, parent[x]);
        }
        return root;
    }

    public static string Kruskal(int n, Edge[] edges)
    {
        Array.Sort(edges, (a, b) => a.W.CompareTo(b.W));
        var parent = new int[n];
        for (int i = 0; i < n; i++)
        {
            parent[i] = i;
        }
        var rank = new byte[n];
        long count = 0, total = 0;
        foreach (Edge e in edges)
        {
            int ru = Find(parent, e.U), rv = Find(parent, e.V);
            if (ru == rv)
            {
                continue;
            }
            if (rank[ru] < rank[rv])
            {
                (ru, rv) = (rv, ru);
            }
            parent[rv] = ru;
            if (rank[ru] == rank[rv])
            {
                rank[ru]++;
            }
            count++;
            total += e.W;
        }
        return Algo.Invariant($"{count}:{total}");
    }
}
