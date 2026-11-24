// Race: window-max (csharp)
// Extracted from suite/csharp/Puzzle.cs — build and run via suite/run.py

using System.Globalization;
using System.Numerics;
using static Kernels.PuzzleText;
namespace Kernels;

public static class Puzzle
{

    internal static string Format(FormattableString text) => text.ToString(CultureInfo.InvariantCulture);

    private static readonly int[] Windows = [10, 100, 1000, 10000, 100000];

    private static string WindowMax(long[] values)
    {
        int n = values.Length;
        var dq = new int[n];
        var hashes = new List<string>(Windows.Length);
        foreach (int w in Windows)
        {
            int head = 0;
            int tail = 0;
            uint h = 0;
            for (int i = 0; i < n; i++)
            {
                long v = values[i];
                while (tail > head && values[dq[tail - 1]] <= v)
                {
                    tail--;
                }
                dq[tail++] = i;
                if (dq[head] <= i - w)
                {
                    head++;
                }
                if (i >= w - 1)
                {
                    h = h * 31 + (uint)values[dq[head]];
                }
            }
            hashes.Add(h.ToString(CultureInfo.InvariantCulture));
        }
        return Format($"{n}:{string.Join(':', hashes)}");
    }

}
