// Race: bfs-grid (csharp)
// Extracted from suite/csharp/AlgoGraphs.cs — build and run via suite/run.py

using System.Runtime.CompilerServices;
namespace Kernels;

public static class AlgoGraphs
{

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

}
