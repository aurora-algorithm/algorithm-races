using System.Diagnostics;
using System.Globalization;

namespace Kernels;

public static class KernelTable
{
    private static readonly Dictionary<string, Func<Options, Measurement>> Kernels = new()
    {
        ["sort-int"] = o => SortKernel(o, Sorts.BuiltIn),
        ["sort-str"] = o =>
            Measure(o, () => Inputs.Strings(o.Size), a => (string[])a.Clone(), Sorts.BuiltIn, StringsChecksum),
        ["quicksort"] = o => SortKernel(o, Sorts.QuickSort),
        ["mergesort"] = o => SortKernel(o, Sorts.MergeSort),
        ["heapsort"] = o => SortKernel(o, Sorts.HeapSort),
        ["radixsort"] = o => SortKernel(o, Sorts.RadixSort),
        ["binary-trees"] = o => Measure(o, BenchmarksGame.BinaryTrees, TextChecksum),
        ["nbody"] = o => Measure(o, BenchmarksGame.NBody, s => s),
        ["mandelbrot"] = o => Measure(o, BenchmarksGame.Mandelbrot, BytesChecksum),
        ["fannkuch"] = o => Measure(o, BenchmarksGame.Fannkuch, s => s),
        ["spectral-norm"] = o => Measure(o, BenchmarksGame.SpectralNorm, s => s),
        ["regex-log"] = o =>
            Measure(o, () => (Text: Inputs.Log(o.Size), Regex: Misc.LogRegex()), x => Misc.RegexLog(x.Regex, x.Text),
                s => s),
        ["hashmap"] = o => Measure(o, () => Inputs.HashMap(o.Size), m => Misc.HashMap(m.Inserts, m.Lookups), s => s),
        ["sha256"] = o => Measure(o, () => Inputs.ShaBuffer(o.Size), Misc.Sha256, s => s),
        ["sieve"] = o => Measure(o, Misc.Sieve, s => s),
        ["matmul"] = o =>
            Measure(o, () => Inputs.Matrices(o.Size), m => Misc.MatMul(m.A, m.B, o.Size), Misc.MatMulChecksum),
        ["json"] = o => Measure(o, () => Inputs.Orders(o.Size, o.DataDir), Misc.Json, Misc.JsonChecksum),
        ["mergesort-mt"] = o => SortKernel(o, a => ParallelKernels.MergeSort(a, o.Threads)),
        ["mandelbrot-mt"] = o => Measure(o, n => ParallelKernels.Mandelbrot(n, o.Threads), BytesChecksum),
        ["binary-trees-mt"] = o => Measure(o, n => ParallelKernels.BinaryTrees(n, o.Threads), TextChecksum),
    };

    public static Func<Options, Measurement>? Find(string name) => Kernels.GetValueOrDefault(name);

    /// <summary>
    /// Builds the input once, then per repeat: a fresh copy (untimed), the timed kernel call. The checksum comes
    /// from the last repeat's result.
    /// </summary>
    private static Measurement Measure<TInput, TResult>(
        Options o,
        Func<TInput> prepare,
        Func<TInput, TInput> fresh,
        Func<TInput, TResult> run,
        Func<TResult, string> check) =>
        MeasureWithExtra(o, prepare, fresh, run, result => new Measurement(check(result), []));

    /// <summary>As Measure, for kernels whose check also reports extra numbers (Measurement.Extra).</summary>
    internal static Measurement MeasureWithExtra<TInput, TResult>(
        Options o,
        Func<TInput> prepare,
        Func<TInput, TInput> fresh,
        Func<TInput, TResult> run,
        Func<TResult, Measurement> check)
    {
        TInput input = prepare();
        var times = new double[o.Repeats];
        TResult result = default!;
        for (int r = 0; r < o.Repeats; r++)
        {
            TInput work = fresh(input);
            long start = Stopwatch.GetTimestamp();
            result = run(work);
            times[r] = Math.Round(Stopwatch.GetElapsedTime(start).TotalMilliseconds, 3);
        }
        return check(result) with { TimesMs = times };
    }

    /// <summary>For kernels that only read their input, so every repeat can share it.</summary>
    internal static Measurement Measure<TInput, TResult>(
        Options o, Func<TInput> prepare, Func<TInput, TResult> run, Func<TResult, string> check) =>
        Measure(o, prepare, input => input, run, check);

    internal static Measurement MeasureWithExtra<TInput, TResult>(
        Options o, Func<TInput> prepare, Func<TInput, TResult> run, Func<TResult, Measurement> check) =>
        MeasureWithExtra(o, prepare, input => input, run, check);

    /// <summary>For kernels whose only input is the size.</summary>
    private static Measurement Measure<TResult>(Options o, Func<int, TResult> run, Func<TResult, string> check) =>
        Measure(o, () => o.Size, run, check);

    private static Measurement SortKernel(Options o, Func<long[], long[]> sort) =>
        Measure(o, () => Inputs.Ints(o.Size), a => (long[])a.Clone(), sort, SortedChecksum);

    private static string SortedChecksum(long[] a) => Checksum.H31(a).ToString(CultureInfo.InvariantCulture);

    private static string StringsChecksum(string[] a) => TextChecksum(string.Concat(a.Select(s => s + "\n")));

    private static string TextChecksum(string s) => Checksum.Fnv1a(s).ToString(CultureInfo.InvariantCulture);

    private static string BytesChecksum(byte[] b) => Checksum.Fnv1a(b).ToString(CultureInfo.InvariantCulture);
}
