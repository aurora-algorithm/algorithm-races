// Race: binary-search (csharp)
// Extracted from suite/csharp/Algo.cs — build and run via suite/run.py

using System.Globalization;
namespace Kernels;

public static class Algo
{

    internal static string Invariant(FormattableString text) => text.ToString(CultureInfo.InvariantCulture);

    private static string Same(string s) => s;

    private static (long[] A, long[] Queries) SortedArray(int n)
    {
        var rng = new XorShift32();
        var a = new long[n];
        long v = 0;
        for (int i = 0; i < n; i++)
        {
            v += 1 + rng.Next() % 4;
            a[i] = v;
        }
        var queries = new long[n];
        for (int i = 0; i < n; i++)
        {
            queries[i] = rng.Next() % (v + 1);
        }
        return (a, queries);
    }

    public static string BinarySearch(long[] a, long[] queries)
    {
        int n = a.Length;
        long hits = 0, total = 0;
        foreach (long q in queries)
        {
            int lo = 0, hi = n;
            while (lo < hi)
            {
                int mid = (lo + hi) / 2;
                if (a[mid] < q)
                {
                    lo = mid + 1;
                }
                else
                {
                    hi = mid;
                }
            }
            if (lo < n && a[lo] == q)
            {
                hits++;
            }
            total += lo;
        }
        return Invariant($"{hits}:{total}");
    }

}
