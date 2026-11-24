// Race: mandelbrot-mt (csharp)
// Extracted from suite/csharp/ParallelKernels.cs — build and run via suite/run.py

namespace Kernels;

public static class ParallelKernels
{

    private static void RunAll(int count, Action<int> work) =>
        Task.WaitAll(Enumerable.Range(0, count).Select(t => Task.Run(() => work(t))));

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

}
