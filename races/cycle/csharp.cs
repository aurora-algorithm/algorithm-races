// Race: cycle (csharp)
// Extracted from suite/csharp/Puzzle.cs — build and run via suite/run.py

using System.Globalization;
using System.Numerics;
using static Kernels.PuzzleText;
namespace Kernels;

public static class Puzzle
{

    internal static string Format(FormattableString text) => text.ToString(CultureInfo.InvariantCulture);

    private const long CycleMod = 67108859;

    private const long CycleC = 12345;

    private static long[] CycleStarts(int n)
    {
        var rng = new XorShift32();
        var starts = new long[n];
        for (int i = 0; i < n; i++)
        {
            starts[i] = rng.Next() % CycleMod;
        }
        return starts;
    }

    private static long Step(long x) => (x * x + CycleC) % CycleMod;

    private static string Cycle(long[] starts)
    {
        long totalMu = 0;
        long totalLam = 0;
        long longest = 0;
        foreach (long x0 in starts)
        {
            long tortoise = Step(x0);
            long hare = Step(tortoise);
            while (tortoise != hare)
            {
                tortoise = Step(tortoise);
                hare = Step(Step(hare));
            }
            long mu = 0;
            tortoise = x0;
            while (tortoise != hare)
            {
                tortoise = Step(tortoise);
                hare = Step(hare);
                mu++;
            }
            long lam = 1;
            hare = Step(tortoise);
            while (tortoise != hare)
            {
                hare = Step(hare);
                lam++;
            }
            totalMu += mu;
            totalLam += lam;
            longest = Math.Max(longest, mu + lam);
        }
        return Format($"{starts.Length}:{totalMu}:{totalLam}:{longest}");
    }

}
