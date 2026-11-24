// Race: life (csharp)
// Extracted from suite/csharp/Puzzle.cs — build and run via suite/run.py

using System.Globalization;
using System.Numerics;
using static Kernels.PuzzleText;
namespace Kernels;

public static class Puzzle
{

    internal static string Format(FormattableString text) => text.ToString(CultureInfo.InvariantCulture);

    private const int LifeSide = 512;

    private static byte[] LifeInput()
    {
        var rng = new XorShift32();
        var grid = new byte[LifeSide * LifeSide];
        for (int i = 0; i < grid.Length; i++)
        {
            grid[i] = (byte)(rng.Next() % 4 == 0 ? 1 : 0);
        }
        return grid;
    }

    private static byte[] Life(byte[] grid, int generations)
    {
        const int w = LifeSide;
        byte[] cur = (byte[])grid.Clone();
        var next = new byte[grid.Length];
        for (int g = 0; g < generations; g++)
        {
            for (int y = 0; y < w; y++)
            {
                int up = (y + w - 1) % w * w;
                int row = y * w;
                int down = (y + 1) % w * w;
                for (int x = 0; x < w; x++)
                {
                    int left = (x + w - 1) % w;
                    int right = (x + 1) % w;
                    int n = cur[up + left] + cur[up + x] + cur[up + right]
                        + cur[row + left] + cur[row + right]
                        + cur[down + left] + cur[down + x] + cur[down + right];
                    next[row + x] = (byte)(n == 3 || (n == 2 && cur[row + x] != 0) ? 1 : 0);
                }
            }
            (cur, next) = (next, cur);
        }
        return cur;
    }

    private static string LifeChecksum(int generations, byte[] grid) =>
        Format($"{generations}:{grid.Sum(b => (long)b)}:{Checksum.Fnv1a(grid)}");

}
