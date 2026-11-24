// Race: binary-trees-mt (csharp)
// Extracted from suite/csharp/ParallelKernels.cs — build and run via suite/run.py

namespace Kernels;

public static class ParallelKernels
{

    private static void RunAll(int count, Action<int> work) =>
        Task.WaitAll(Enumerable.Range(0, count).Select(t => Task.Run(() => work(t))));

    public static string BinaryTrees(int n, int threads) =>
        BenchmarksGame.BinaryTreesLines(n, (depth, iterations) =>
        {
            var checks = new int[threads];
            RunAll(threads, t => checks[t] = BenchmarksGame.DepthChecks(depth, Share(iterations, threads, t)));
            return checks.Sum();
        });

    private static int Share(int total, int parts, int index) => total / parts + (index < total % parts ? 1 : 0);

}
