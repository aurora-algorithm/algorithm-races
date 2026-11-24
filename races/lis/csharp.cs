// Race: lis (csharp)
// Extracted from suite/csharp/Algo.cs — build and run via suite/run.py

using System.Globalization;
namespace Kernels;

public static class Algo
{

    internal static string Invariant(FormattableString text) => text.ToString(CultureInfo.InvariantCulture);

    private static string Same(string s) => s;

    public static string Lis(long[] values)
    {
        var tails = new long[16];
        int length = 0;
        foreach (long x in values)
        {
            int i = Array.BinarySearch(tails, 0, length, x);
            if (i < 0)
            {
                i = ~i;
            }
            if (i == tails.Length)
            {
                Array.Resize(ref tails, tails.Length * 2);
            }
            tails[i] = x;
            if (i == length)
            {
                length++;
            }
        }
        return Invariant($"{length}:{Checksum.H31(tails.Take(length))}");
    }

}
