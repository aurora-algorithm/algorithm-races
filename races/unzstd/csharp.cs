// Race: unzstd (csharp)
// Extracted from suite/csharp/Work.cs — build and run via suite/run.py

using System.Globalization;
using System.IO.Compression;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using CsvHelper;
using Konscious.Security.Cryptography;
using SkiaSharp;
using SylvanCsvReader = Sylvan.Data.Csv.CsvDataReader;
namespace Kernels;

public static class Work
{

    private static string DataFile(Options o, string pattern) =>
        Path.Combine(o.DataDir, string.Format(CultureInfo.InvariantCulture, pattern, o.Size));

    private static byte[] Corpus(Options o) => File.ReadAllBytes(DataFile(o, "corpus-{0}.bin"));

    private static string PlainChecksum(byte[] raw) =>
        $"{raw.Length}:{Convert.ToHexStringLower(SHA256.HashData(raw))}";

    private static Measurement DecompressKernel(Options o, Func<byte[], byte[]> compress, Func<byte[], byte[]> decompress) =>
        KernelTable.Measure(o, () => compress(Corpus(o)), decompress, PlainChecksum);

    private static byte[] Zstd(byte[] raw)
    {
        using var compressor = new ZstdSharp.Compressor(3);
        return compressor.Wrap(raw).ToArray();
    }

    private static byte[] Unzstd(byte[] packed)
    {
        using var decompressor = new ZstdSharp.Decompressor();
        return decompressor.Unwrap(packed).ToArray();
    }

}
