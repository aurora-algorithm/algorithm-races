// Race: bmap-str.go.map (csharp)
// Extracted from suite/csharp/WaysMaps.cs — build and run via suite/run.py

using System.Collections.Concurrent;
using System.Collections.Frozen;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Text;
namespace Kernels;

public static class WaysMaps
{

    /// <summary>Even i looks up a key that is in the map; odd i the key without its last letter.</summary>
    private static (string[], string[]) StringKeys(int n)
    {
        string[] inserts = Inputs.Strings(n);
        string[] lookups = inserts.Select((key, i) => i % 2 == 0 ? key : key[..^1]).ToArray();
        return (inserts, lookups);
    }

    /// <summary>
    /// Every repeat gets new string objects with the same text, for the inserts and separately for the lookups (made
    /// untimed): equal keys are never the same reference, so no lookup takes the reference-equality shortcut.
    /// </summary>
    private static (string[], string[]) FreshStrings((string[] Inserts, string[] Lookups) kv) =>
        (CopyAll(kv.Inserts), CopyAll(kv.Lookups));

    private static string[] CopyAll(string[] keys) => Array.ConvertAll(keys, k => new string(k.AsSpan()));

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
