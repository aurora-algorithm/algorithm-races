// Race: gzip (csharp)
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

    private static Measurement CompressKernel(Options o, Func<byte[], byte[]> compress, Func<byte[], byte[]> decompress) =>
        KernelTable.MeasureWithExtra(o, () => Corpus(o), compress, packed =>
            new Measurement(PlainChecksum(decompress(packed)), [], new() { ["compressed_bytes"] = packed.Length }));

    private static byte[] Gzip(byte[] raw)
    {
        using var output = new MemoryStream();
        using (var gzip = new GZipStream(output, new ZLibCompressionOptions { CompressionLevel = 6 }))
        {
            gzip.Write(raw);
        }
        return output.ToArray();
    }

    private static byte[] Gunzip(byte[] packed)
    {
        using var gzip = new GZipStream(new MemoryStream(packed), CompressionMode.Decompress);
        using var output = new MemoryStream();
        gzip.CopyTo(output);
        return output.ToArray();
    }

}
