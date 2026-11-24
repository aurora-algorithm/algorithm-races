// Race: heapsort (csharp)
// Extracted from suite/csharp/Sorts.cs — build and run via suite/run.py

namespace Kernels;

public static class Sorts
{

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

}
