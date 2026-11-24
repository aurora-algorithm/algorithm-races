using System.Globalization;

namespace Kernels;

/// <summary>
/// The builtins group (kernels/builtins-NOTES.md): the ways .NET ships to do one job, one way per kernel, named
/// JOB.csharp.WAY. Every way of a job gets the same input and must print the same checksum.
/// </summary>
public static class Ways
{
    private const string Lang = "csharp";

    private static readonly Dictionary<string, Func<int, long[]>> IntInputs = new()
    {
        ["bsort-int"] = Inputs.Ints,
        ["bsort-int-sorted"] = n => Sorted(Inputs.Ints(n), null),
        ["bsort-int-reversed"] = n => Reversed(Sorted(Inputs.Ints(n), null)),
        ["bsort-int64"] = Inputs.Ints64,
    };

    private static readonly Dictionary<string, Func<int, string[]>> StringInputs = new()
    {
        ["bsort-str"] = Inputs.Strings,
        ["bsort-str-sorted"] = n => Sorted(Inputs.Strings(n), StringComparer.Ordinal),
        ["bsort-str-reversed"] = n => Reversed(Sorted(Inputs.Strings(n), StringComparer.Ordinal)),
    };

    private static readonly Dictionary<string, Func<Options, Measurement>> Kernels = Build();

    // Integers sort with comparer null: every call below then takes its default-comparer path, the same code as
    // Array.Sort(a), a.Order() and so on. Strings always pass StringComparer.Ordinal (the default is culture-aware).

    public static Func<Options, Measurement>? Find(string name) => Kernels.GetValueOrDefault(name);

    private static Dictionary<string, Func<Options, Measurement>> Build()
    {
        var table = new Dictionary<string, Func<Options, Measurement>>();
        foreach (var (job, make) in IntInputs)
        {
            AddSorts(table, job, make, null, a => Plain(H31(a)));
            // Library way: HPCsharp 3.19.8 SortRadixLsd, an LSD radix sort (its scratch buffer is allocated inside the call).
            table[$"{job}.{Lang}.HPCsharp.SortRadixLsd"] = o => KernelTable.MeasureWithExtra(
                o, () => make(o.Size), a => (long[])a.Clone(), a =>
                {
                    HPCsharp.Algorithm.SortRadixLsd(a);
                    return a;
                }, a => Plain(H31(a)));
        }
        foreach (var (job, make) in StringInputs)
        {
            AddSorts(table, job, make, StringComparer.Ordinal, a => Plain(StringsChecksum(a)));
        }
        WaysMaps.Add(table, Lang);
        return table;
    }

    private static void AddSorts<T>(
        Dictionary<string, Func<Options, Measurement>> table,
        string job,
        Func<int, T[]> make,
        IComparer<T>? comparer,
        Func<IEnumerable<T>, Measurement> check)
    {
        string Name(string way) => $"{job}.{Lang}.{way}";
        table[Name("Array.Sort")] = o => ArraySort(o, make, comparer, check);
        table[Name("Span.Sort")] = o => SpanSort(o, make, comparer, check);
        table[Name("List.Sort")] = o => KernelTable.MeasureWithExtra(
            o, () => new List<T>(make(o.Size)), list => new List<T>(list), list => SortList(list, comparer), check);
        table[Name("LINQ.OrderBy")] = o => Linq(o, make, a => a.OrderBy(x => x, comparer).ToArray(), check);
        table[Name("LINQ.Order")] = o => Linq(o, make, a => a.Order(comparer).ToArray(), check);
        table[Name("PLINQ.OrderBy")] = o => WithPoolThreads(
            Linq(o, make, a => a.AsParallel().OrderBy(x => x, comparer).ToArray(), check));
    }

    private static Measurement ArraySort<T>(
        Options o, Func<int, T[]> make, IComparer<T>? comparer, Func<IEnumerable<T>, Measurement> check) =>
        KernelTable.MeasureWithExtra(o, () => make(o.Size), a => (T[])a.Clone(), a =>
        {
            Array.Sort(a, comparer);
            return a;
        }, check);

    private static Measurement SpanSort<T>(
        Options o, Func<int, T[]> make, IComparer<T>? comparer, Func<IEnumerable<T>, Measurement> check) =>
        KernelTable.MeasureWithExtra(o, () => make(o.Size), a => (T[])a.Clone(), a =>
        {
            a.AsSpan().Sort(comparer);
            return a;
        }, check);

    private static Measurement Linq<T>(
        Options o, Func<int, T[]> make, Func<T[], T[]> sort, Func<IEnumerable<T>, Measurement> check) =>
        KernelTable.MeasureWithExtra(o, () => make(o.Size), sort, check);

    private static List<T> SortList<T>(List<T> list, IComparer<T>? comparer)
    {
        list.Sort(comparer);
        return list;
    }

    /// <summary>PLINQ's stock degree of parallelism is Environment.ProcessorCount (the affinity mask).</summary>
    private static Measurement WithPoolThreads(Measurement m) =>
        m with { Extra = new() { ["pool_threads"] = Environment.ProcessorCount } };

    private static T[] Sorted<T>(T[] a, IComparer<T>? comparer)
    {
        Array.Sort(a, comparer);
        return a;
    }

    private static T[] Reversed<T>(T[] a)
    {
        Array.Reverse(a);
        return a;
    }

    internal static Measurement Plain(string checksum) => new(checksum, []);

    private static string H31(IEnumerable<long> a) => Checksum.H31(a).ToString(CultureInfo.InvariantCulture);

    private static string StringsChecksum(IEnumerable<string> a) =>
        Checksum.Fnv1a(string.Concat(a.Select(s => s + "\n"))).ToString(CultureInfo.InvariantCulture);
}
