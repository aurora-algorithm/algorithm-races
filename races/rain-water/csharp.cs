// Race: rain-water (csharp)
// Extracted from suite/csharp/Puzzle.cs — build and run via suite/run.py

using System.Globalization;
using System.Numerics;
using static Kernels.PuzzleText;
namespace Kernels;

public static class Puzzle
{

    internal static string Format(FormattableString text) => text.ToString(CultureInfo.InvariantCulture);

    private static int[] Terrain(int side)
    {
        var rng = new XorShift32();
        var heights = new int[side * side];
        for (int i = 0; i < heights.Length; i++)
        {
            heights[i] = (int)(rng.Next() % 1000);
        }
        return heights;
    }

    private static string RainWater(int[] heights)
    {
        int n = heights.Length;
        int side = (int)Math.Sqrt(n);
        var seen = new bool[n];
        var heap = new PriorityQueue<long, long>();
        for (int idx = 0; idx < n; idx++)
        {
            int r = idx / side;
            int c = idx % side;
            if (r == 0 || r == side - 1 || c == 0 || c == side - 1)
            {
                seen[idx] = true;
                long key = (long)heights[idx] * n + idx;
                heap.Enqueue(key, key);
            }
        }
        long water = 0;
        long wet = 0;
        (int Dr, int Dc)[] moves = [(-1, 0), (1, 0), (0, -1), (0, 1)];
        while (heap.Count > 0)
        {
            long top = heap.Dequeue();
            long level = top / n;
            int idx = (int)(top % n);
            int r = idx / side;
            int c = idx % side;
            foreach ((int dr, int dc) in moves)
            {
                int nr = r + dr;
                int nc = c + dc;
                if (nr < 0 || nr >= side || nc < 0 || nc >= side)
                {
                    continue;
                }
                int neighbour = nr * side + nc;
                if (seen[neighbour])
                {
                    continue;
                }
                seen[neighbour] = true;
                long h = heights[neighbour];
                if (h < level)
                {
                    water += level - h;
                    wet++;
                    h = level;
                }
                long key = h * n + neighbour;
                heap.Enqueue(key, key);
            }
        }
        return Format($"{side}:{water}:{wet}");
    }

}
