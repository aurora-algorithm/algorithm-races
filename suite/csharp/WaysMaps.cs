using System.Collections.Concurrent;
using System.Collections.Frozen;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Text;

namespace Kernels;

/// <summary>
/// The builtins group's hash map and string-join jobs (kernels/builtins-NOTES.md). Maps use the default comparer
/// and capacity: for string keys that is ordinal equality.
/// </summary>
public static class WaysMaps
{
    internal static void Add(Dictionary<string, Func<Options, Measurement>> table, string lang)
    {
        AddMaps(table, $"bmap-int.{lang}", IntKeys);
        AddMaps(table, $"bmap-str.{lang}", StringKeys, FreshStrings);
        table[$"bjoin.{lang}.string.Join"] = o => Join(o, parts => string.Join(',', parts));
        table[$"bjoin.{lang}.StringBuilder"] = o => Join(o, parts => Build(new StringBuilder(), parts));
        table[$"bjoin.{lang}.StringBuilder.capacity"] = o => Join(o, BuildWithCapacity);
    }

    private static void AddMaps<TKey>(
        Dictionary<string, Func<Options, Measurement>> table,
        string prefix,
        Func<int, (TKey[], TKey[])> make,
        Func<(TKey[], TKey[]), (TKey[], TKey[])>? fresh = null)
        where TKey : notnull
    {
        Func<(TKey[], TKey[]), (TKey[], TKey[])> copy = fresh ?? (kv => kv);
        table[$"{prefix}.Dictionary"] = o => Map(o, make, copy, CountDictionary);
        table[$"{prefix}.CollectionsMarshal.GetValueRefOrAddDefault"] = o => Map(o, make, copy, CountByRef);
        table[$"{prefix}.FrozenDictionary"] = o => Map(o, make, copy, CountFrozen);
        table[$"{prefix}.ConcurrentDictionary"] = o => Map(o, make, copy, CountConcurrent);
    }

    private static (long[], long[]) IntKeys(int n) => Inputs.HashMap(n);

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

    private static string CountDictionary<TKey>(TKey[] inserts, TKey[] lookups) where TKey : notnull
    {
        Dictionary<TKey, long> counts = Counted(inserts);
        long hits = 0;
        foreach (TKey k in lookups)
        {
            if (counts.ContainsKey(k))
            {
                hits++;
            }
        }
        return Result(counts.Count, hits);
    }

    private static string CountByRef<TKey>(TKey[] inserts, TKey[] lookups) where TKey : notnull
    {
        var counts = new Dictionary<TKey, long>();
        foreach (TKey k in inserts)
        {
            CollectionsMarshal.GetValueRefOrAddDefault(counts, k, out _)++;
        }
        long hits = 0;
        foreach (TKey k in lookups)
        {
            if (counts.ContainsKey(k))
            {
                hits++;
            }
        }
        return Result(counts.Count, hits);
    }

    private static string CountFrozen<TKey>(TKey[] inserts, TKey[] lookups) where TKey : notnull
    {
        FrozenDictionary<TKey, long> counts = Counted(inserts).ToFrozenDictionary();
        long hits = 0;
        foreach (TKey k in lookups)
        {
            if (counts.ContainsKey(k))
            {
                hits++;
            }
        }
        return Result(counts.Count, hits);
    }

    private static string CountConcurrent<TKey>(TKey[] inserts, TKey[] lookups) where TKey : notnull
    {
        var counts = new ConcurrentDictionary<TKey, long>();
        foreach (TKey k in inserts)
        {
            counts.AddOrUpdate(k, 1, (_, v) => v + 1);
        }
        long hits = 0;
        foreach (TKey k in lookups)
        {
            if (counts.ContainsKey(k))
            {
                hits++;
            }
        }
        return Result(counts.Count, hits);
    }

    private static Measurement Join(Options o, Func<string[], string> join) =>
        KernelTable.Measure(o, () => Inputs.Ints(o.Size).Select(v => v.ToString(CultureInfo.InvariantCulture)).ToArray(),
            join, Joined);

    private static string Build(StringBuilder text, string[] parts)
    {
        for (int i = 0; i < parts.Length; i++)
        {
            if (i > 0)
            {
                text.Append(',');
            }
            text.Append(parts[i]);
        }
        return text.ToString();
    }

    private static string BuildWithCapacity(string[] parts)
    {
        int total = Math.Max(parts.Length - 1, 0);
        foreach (string p in parts)
        {
            total += p.Length;
        }
        return Build(new StringBuilder(total), parts);
    }

    private static string Joined(string s) =>
        $"{Encoding.UTF8.GetByteCount(s)}:{Checksum.Fnv1a(s).ToString(CultureInfo.InvariantCulture)}";
}
