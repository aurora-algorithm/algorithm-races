// Race: sort-int (csharp)
// Extracted from suite/csharp/Sorts.cs — build and run via suite/run.py

namespace Kernels;

public static class Sorts
{

    public static long[] BuiltIn(long[] a)
    {
        Array.Sort(a);
        return a;
    }

    public static string[] BuiltIn(string[] a)
    {
        Array.Sort(a, StringComparer.Ordinal);
        return a;
    }

}
