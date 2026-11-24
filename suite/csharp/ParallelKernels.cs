namespace Kernels;

/// <summary>The multi-threaded kernels, run as tasks on the shared thread pool.</summary>
public static class ParallelKernels
{
    /// <summary>
    /// Makes sure the pool holds <paramref name="threads"/> started worker threads before timing: the pool creates
    /// threads on demand, so each task waits at a barrier until all of them run at once.
    /// </summary>
    public static void WarmUp(int threads)
    {
        ThreadPool.GetMinThreads(out int workers, out int io);
        if (workers < threads)
        {
            ThreadPool.SetMinThreads(threads, io);
        }
        using var barrier = new Barrier(threads);
        RunAll(threads, _ => barrier.SignalAndWait());
    }

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

    public static byte[] Mandelbrot(int n, int threads)
    {
        var bitmap = new byte[n * BenchmarksGame.RowBytes(n)];
        RunAll(threads, t =>
        {
            for (int y = t; y < n; y += threads)
            {
                BenchmarksGame.MandelbrotRow(n, y, bitmap);
            }
        });
        return bitmap;
    }

    public static string BinaryTrees(int n, int threads) =>
        BenchmarksGame.BinaryTreesLines(n, (depth, iterations) =>
        {
            var checks = new int[threads];
            RunAll(threads, t => checks[t] = BenchmarksGame.DepthChecks(depth, Share(iterations, threads, t)));
            return checks.Sum();
        });

    private static int Share(int total, int parts, int index) => total / parts + (index < total % parts ? 1 : 0);
}
