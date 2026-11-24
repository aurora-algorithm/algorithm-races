// Race: bmap-int.go.map (csharp)
// Extracted from suite/csharp/WaysMaps.cs — build and run via suite/run.py

using System.Collections.Concurrent;
using System.Collections.Frozen;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Text;
namespace Kernels;

public static class WaysMaps
{

    private static (long[], long[]) IntKeys(int n) => Inputs.HashMap(n);

    private static Measurement Map<TKey>(
        Options o,
        Func<int, (TKey[], TKey[])> make,
        Func<(TKey[], TKey[]), (TKey[], TKey[])> fresh,
        Func<TKey[], TKey[], string> count) =>
        KernelTable.MeasureWithExtra(o, () => make(o.Size), fresh, kv => count(kv.Item1, kv.Item2),
            s => new Measurement(s, []));

    private static string Result(int count, long hits) => $"{count}:{hits.ToString(CultureInfo.InvariantCulture)}";

    private static Dictionary<TKey, long> Counted<TKey>(TKey[] inserts) where TKey : notnull
    {
        var counts = new Dictionary<TKey, long>();
        foreach (TKey k in inserts)
        {
            counts[k] = counts.GetValueOrDefault(k) + 1;
        }
        return counts;
    }

}
