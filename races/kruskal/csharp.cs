// Race: kruskal (csharp)
// Extracted from suite/csharp/AlgoGraphs.cs — build and run via suite/run.py

using System.Runtime.CompilerServices;
namespace Kernels;

public static class AlgoGraphs
{

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
