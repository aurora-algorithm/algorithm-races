// Race: collatz (csharp)
// Extracted from suite/csharp/Puzzle.cs — build and run via suite/run.py

using System.Globalization;
using System.Numerics;
using static Kernels.PuzzleText;
namespace Kernels;

public static class Puzzle
{

    internal static string Format(FormattableString text) => text.ToString(CultureInfo.InvariantCulture);

    private static string Collatz(int n)
    {
        var cache = new int[n];
        cache[1] = 1;
        long best = 1;
        long bestLength = 1;
        long total = 1;
        for (int start = 2; start < n; start++)
        {
            long x = start;
            int steps = 0;
            while (x >= start)
            {
                x = x % 2 == 0 ? x / 2 : 3 * x + 1;
                steps++;
            }
            int length = steps + cache[x];
            cache[start] = length;
            total += length;
            if (length > bestLength)
            {
                best = start;
                bestLength = length;
            }
        }
        return Format($"{n}:{best}:{bestLength}:{total}");
    }

}
