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

    public static long[] HeapSort(long[] a)
    {
        int n = a.Length;
        for (int start = n / 2 - 1; start >= 0; start--)
        {
            SiftDown(a, start, n);
        }
        for (int end = n - 1; end > 0; end--)
        {
            (a[0], a[end]) = (a[end], a[0]);
            SiftDown(a, 0, end);
        }
        return a;
    }

    private static void SiftDown(long[] a, int root, int end)
    {
        while (2 * root + 1 < end)
        {
            int child = 2 * root + 1;
            if (child + 1 < end && a[child] < a[child + 1])
            {
                child++;
            }
            if (a[root] < a[child])
            {
                (a[root], a[child]) = (a[child], a[root]);
                root = child;
            }
            else
            {
                return;
            }
        }
    }

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
