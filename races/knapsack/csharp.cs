// Race: knapsack (csharp)
// Extracted from suite/csharp/Algo.cs — build and run via suite/run.py

using System.Globalization;
namespace Kernels;

public static class Algo
{

    private const int KnapsackItems = 1000;

    internal static string Invariant(FormattableString text) => text.ToString(CultureInfo.InvariantCulture);

    private static string Same(string s) => s;

    private static (int Weight, long Value)[] Items()
    {
        var rng = new XorShift32();
        var items = new (int Weight, long Value)[KnapsackItems];
        for (int i = 0; i < items.Length; i++)
        {
            int weight = 1 + (int)(rng.Next() % 1000);
            items[i] = (weight, 1 + rng.Next() % 1000);
        }
        return items;
    }

    public static string Knapsack(int capacity, (int Weight, long Value)[] items)
    {
        var best = new long[capacity + 1];
        foreach ((int weight, long value) in items)
        {
            for (int c = capacity; c >= weight; c--)
            {
                if (best[c - weight] + value > best[c])
                {
                    best[c] = best[c - weight] + value;
                }
            }
        }
        return Invariant($"{best[capacity]}:{Checksum.H31(best)}");
    }

}
