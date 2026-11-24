// Race: image (csharp)
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

    private static (int, int, int) SkiaConvert(byte[] data)
    {
        using SKBitmap source = SKBitmap.Decode(data);
        var info = new SKImageInfo(TargetWidth, TargetHeight(source.Width, source.Height));
        using SKBitmap small = source.Resize(info, new SKSamplingOptions(SKCubicResampler.CatmullRom));
        using SKData jpeg = small.Encode(SKEncodedImageFormat.Jpeg, JpegQuality);
        return (small.Width, small.Height, (int)jpeg.Size);
    }

}
