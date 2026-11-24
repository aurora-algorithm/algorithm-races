// Race: bsort-int.go.zermelo.Sort (csharp)
// Extracted from suite/csharp/Ways.cs — build and run via suite/run.py

using System.Globalization;
namespace Kernels;

public static class Ways
{

    private static readonly Dictionary<string, Func<int, long[]>> IntInputs = new()
    {
        ["bsort-int"] = Inputs.Ints,
        ["bsort-int-sorted"] = n => Sorted(Inputs.Ints(n), null),
        ["bsort-int-reversed"] = n => Reversed(Sorted(Inputs.Ints(n), null)),
        ["bsort-int64"] = Inputs.Ints64,
    };

    private static Measurement ArraySort<T>(
        Options o, Func<int, T[]> make, IComparer<T>? comparer, Func<IEnumerable<T>, Measurement> check) =>
        KernelTable.MeasureWithExtra(o, () => make(o.Size), a => (T[])a.Clone(), a =>
        {
            Array.Sort(a, comparer);
            return a;
        }, check);

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

}
