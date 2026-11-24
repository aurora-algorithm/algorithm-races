using System.Runtime;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text.Json;
using Kernels;

// usage: Kernels KERNEL --size N [--repeats R] [--threads T] [--data DIR]
// Prints one JSON line (kernels/SPEC.md section 1).

Options options;
try
{
    options = Options.Parse(args);
}
catch (ArgumentException e)
{
    Console.Error.WriteLine(e.Message);
    Console.Error.WriteLine("usage: Kernels KERNEL --size N [--repeats R] [--threads T] [--data DIR]");
    return 2;
}

Func<Options, Measurement>? kernel = KernelTable.Find(options.Kernel) ?? Work.Find(options.Kernel)
    ?? Algo.Find(options.Kernel) ?? Puzzle.Find(options.Kernel) ?? Ways.Find(options.Kernel)
    ?? Arena.Find(options.Kernel);
if (kernel is null)
{
    Console.Error.WriteLine($"unknown kernel {options.Kernel}");
    return 2;
}

if (options.Kernel.EndsWith("-mt", StringComparison.Ordinal))
{
    ParallelKernels.WarmUp(options.Threads);
}

Measurement measurement = kernel(options);
string runtime = Work.Find(options.Kernel) is null ? RuntimeName() : $"{RuntimeName()} {Work.Libraries()}";
var result = new KernelResult(
    options.Kernel, options.Size, options.Threads, measurement.Checksum, measurement.TimesMs, runtime,
    measurement.Extra);
Console.WriteLine(JsonSerializer.Serialize(result, KernelsJson.Default.KernelResult));
return 0;

static string RuntimeName()
{
    string compiler = RuntimeFeature.IsDynamicCodeSupported ? "JIT" : "NativeAOT";
    string gc = GCSettings.IsServerGC ? "server GC" : "workstation GC";
    return $"{RuntimeInformation.FrameworkDescription} {compiler} {gc} {RuntimeInformation.ProcessArchitecture}";
}
