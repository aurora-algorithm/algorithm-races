// Race: mergesort (csharp)
// Extracted from suite/csharp/Sorts.cs — build and run via suite/run.py

namespace Kernels;

public static class Sorts
{

    public static long[] MergeSort(long[] a)
    {
        MergeSort(a, new long[a.Length], 0, a.Length);
        return a;
    }

    /// <summary>Top-down, half-open [lo, hi).</summary>
    public static void MergeSort(long[] a, long[] aux, int lo, int hi)
    {
        if (hi - lo < 2)
        {
            return;
        }
        int mid = lo + (hi - lo) / 2;
        MergeSort(a, aux, lo, mid);
        MergeSort(a, aux, mid, hi);
        Merge(a, aux, lo, mid, hi);
    }

    public static void Merge(long[] a, long[] aux, int lo, int mid, int hi)
    {
        Array.Copy(a, lo, aux, lo, hi - lo);
        int i = lo, j = mid, k = lo;
        while (i < mid && j < hi)
        {
            a[k++] = aux[j] < aux[i] ? aux[j++] : aux[i++];
        }
        while (i < mid)
        {
            a[k++] = aux[i++];
        }
        while (j < hi)
        {
            a[k++] = aux[j++];
        }
    }

}
