// Race: mergesort-mt (csharp)
// Extracted from suite/csharp/ParallelKernels.cs — build and run via suite/run.py

namespace Kernels;

public static class ParallelKernels
{

    private static void RunAll(int count, Action<int> work) =>
        Task.WaitAll(Enumerable.Range(0, count).Select(t => Task.Run(() => work(t))));

    public static long[] MergeSort(long[] a, int threads)
    {
        var aux = new long[a.Length];
        List<int> bounds = [.. Enumerable.Range(0, threads + 1).Select(t => (int)((long)t * a.Length / threads))];
        RunAll(threads, t => Sorts.MergeSort(a, aux, bounds[t], bounds[t + 1]));
        while (bounds.Count > 2)
        {
            int pairs = (bounds.Count - 1) / 2;
            List<int> current = bounds;
            RunAll(pairs, p => Sorts.Merge(a, aux, current[2 * p], current[2 * p + 1], current[2 * p + 2]));
            bounds = [.. current.Where((_, i) => i % 2 == 0 || i == current.Count - 1)];
        }
        return a;
    }

}
