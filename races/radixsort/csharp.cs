// Race: radixsort (csharp)
// Extracted from suite/csharp/Sorts.cs — build and run via suite/run.py

namespace Kernels;

public static class Sorts
{

    /// <summary>LSD, 8 bits per pass; after the 4 passes the result is back in <paramref name="a"/>.</summary>
    public static long[] RadixSort(long[] a)
    {
        long[] buffer = new long[a.Length];
        for (int shift = 0; shift < 32; shift += 8)
        {
            var count = new int[256];
            foreach (long v in a)
            {
                count[(v >> shift) & 255]++;
            }
            var position = new int[256];
            int total = 0;
            for (int digit = 0; digit < 256; digit++)
            {
                position[digit] = total;
                total += count[digit];
            }
            foreach (long v in a)
            {
                buffer[position[(v >> shift) & 255]++] = v;
            }
            (a, buffer) = (buffer, a);
        }
        return a;
    }

}
