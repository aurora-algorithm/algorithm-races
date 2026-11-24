// Race: bsort-int-reversed.goose.sort (csharp)
// Extracted from suite/csharp/Ways.cs — build and run via suite/run.py

using System.Globalization;
namespace Kernels;

public static class Ways
{

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

    private static T[] Reversed<T>(T[] a)
    {
        Array.Reverse(a);
        return a;
    }

}
