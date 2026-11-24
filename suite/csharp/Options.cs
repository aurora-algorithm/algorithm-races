using System.Globalization;
using System.Text.Json.Serialization;

namespace Kernels;

public sealed record KernelResult(
    string Kernel,
    int Size,
    int Threads,
    string Checksum,
    double[] TimesMs,
    string Runtime,
    [property: JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)] Dictionary<string, long>? Extra);

/// <summary>Extra: numbers beside the checksum that differ by library (output sizes of the work kernels).</summary>
public sealed record Measurement(string Checksum, double[] TimesMs, Dictionary<string, long>? Extra = null);

public sealed record Options(string Kernel, int Size, int Repeats, int Threads, string DataDir)
{
    public static Options Parse(string[] args)
    {
        if (args.Length == 0 || args[0].StartsWith('-'))
        {
            throw new ArgumentException("missing KERNEL");
        }
        int? size = null;
        int repeats = 1, threads = 1;
        string dataDir = ".";
        for (int i = 1; i < args.Length; i += 2)
        {
            if (i + 1 >= args.Length)
            {
                throw new ArgumentException($"{args[i]} needs a value");
            }
            string value = args[i + 1];
            switch (args[i])
            {
                case "--size": size = PositiveInt(args[i], value); break;
                case "--repeats": repeats = PositiveInt(args[i], value); break;
                case "--threads": threads = PositiveInt(args[i], value); break;
                case "--data": dataDir = value; break;
                default: throw new ArgumentException($"unknown option {args[i]}");
            }
        }
        int n = size ?? throw new ArgumentException("--size is required");
        return new Options(args[0], n, repeats, threads, dataDir);
    }

    private static int PositiveInt(string name, string value) =>
        int.TryParse(value, NumberStyles.None, CultureInfo.InvariantCulture, out int n) && n > 0
            ? n
            : throw new ArgumentException($"{name} must be a positive integer, got '{value}'");
}
