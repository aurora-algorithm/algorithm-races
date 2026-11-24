// Race: quicksort (csharp)
// Extracted from suite/csharp/Sorts.cs — build and run via suite/run.py

namespace Kernels;

public static class Sorts
{

    public static long[] QuickSort(long[] a)
    {
        QuickSort(a, 0, a.Length - 1);
        return a;
    }

    /// <summary>Hoare partition, middle pivot, inclusive bounds; recurses into the smaller side.</summary>
    private static void QuickSort(long[] a, int lo, int hi)
    {
        while (lo < hi)
        {
            long pivot = a[lo + (hi - lo) / 2];
            int i = lo - 1;
            int j = hi + 1;
            while (true)
            {
                do i++; while (a[i] < pivot);
                do j--; while (a[j] > pivot);
                if (i >= j)
                {
                    break;
                }
                (a[i], a[j]) = (a[j], a[i]);
            }
            if (j - lo < hi - j)
            {
                QuickSort(a, lo, j);
                lo = j + 1;
            }
            else
            {
                QuickSort(a, j + 1, hi);
                hi = j;
            }
        }
    }

}
