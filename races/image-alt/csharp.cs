// Race: image-alt (csharp)
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

    private const int TargetWidth = 1080;

    private const int JpegQuality = 80;

    private static int TargetHeight(int width, int height) => (height * TargetWidth + width / 2) / width;

    private sealed record ImageResult(int Count, int Width, int Height, long OutBytes);

    private static Measurement ImageKernel(Options o, Func<byte[], (int Width, int Height, int Bytes)> convert)
    {
        Func<byte[][]> load = () => Enumerable.Range(0, o.Size)
            .Select(i => File.ReadAllBytes(Path.Combine(o.DataDir, "photos", $"photo-{i % Photos}.jpg")))
            .ToArray();
        return KernelTable.MeasureWithExtra(o, load, photos =>
        {
            ImageResult r = new(photos.Length, 0, 0, 0);
            foreach (byte[] data in photos)
            {
                (int width, int height, int bytes) = convert(data);
                r = r with { Width = width, Height = height, OutBytes = r.OutBytes + bytes };
            }
            return r;
        }, r => new Measurement($"{r.Count}:{r.Width}x{r.Height}", [], new() { ["out_bytes"] = r.OutBytes }));
    }

    /// <summary>libvips' thumbnail: shrink-on-load (the JPEG decoder skips detail) then Lanczos3.</summary>
    private static (int, int, int) VipsConvert(byte[] data)
    {
        // libvips caches operations by their arguments; a warm repeat on the same buffer would be a cache hit.
        NetVips.Cache.Max = 0;
        using NetVips.Image small = NetVips.Image.ThumbnailBuffer(data, TargetWidth);
        byte[] jpeg = small.JpegsaveBuffer(q: JpegQuality);
        return (small.Width, small.Height, jpeg.Length);
    }

}
