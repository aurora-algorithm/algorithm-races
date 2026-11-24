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

/// <summary>The "work" kernels (kernels/SPEC.md section 11): images, compression, passwords, CSV, word count.</summary>
public static class Work
{
    private const int Photos = 8;
    private const int TargetWidth = 1080;
    private const int JpegQuality = 80;
    private const int BcryptCost = 10;
    private const int Argon2Memory = 19456;
    private const int Argon2Time = 2;
    private const int Argon2Lanes = 1;
    private const string RefPassword = "correct horse battery staple";
    private const string BcryptRef = "$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy";
    private const string Argon2Ref =
        "$argon2id$v=19$m=19456,t=2,p=1$c3RhY2tzLWJlbmNoLXNhbHQ$oLeQRnXjHhLMJZcTdYDeVr93DW7srzPjTWhPG3BTryk";

    private static readonly string Argon2Prefix = $"$argon2id$v=19$m={Argon2Memory},t={Argon2Time},p={Argon2Lanes}$";

    private static readonly Dictionary<string, Func<Options, Measurement>> Kernels = new()
    {
        ["image"] = o => ImageKernel(o, SkiaConvert),
        ["image-alt"] = o => ImageKernel(o, VipsConvert),
        ["gzip"] = o => CompressKernel(o, Gzip, Gunzip),
        ["gunzip"] = o => DecompressKernel(o, Gzip, Gunzip),
        ["zstd"] = o => CompressKernel(o, Zstd, Unzstd),
        ["unzstd"] = o => DecompressKernel(o, Zstd, Unzstd),
        ["bcrypt"] = o => KernelTable.Measure(o, () => Passwords(o.Size), BcryptHashAll, BcryptChecksum),
        ["argon2id"] = o => KernelTable.Measure(o, () => Passwords(o.Size), Argon2HashAll, Argon2Checksum),
        ["csv"] = o => KernelTable.Measure(o, () => DataFile(o, "orders-{0}.csv"), CsvHelperTotals, CsvChecksum),
        ["csv-alt"] = o => KernelTable.Measure(o, () => DataFile(o, "orders-{0}.csv"), SylvanTotals, CsvChecksum),
        ["wordcount"] = o => KernelTable.Measure(o, () => DataFile(o, "text-{0}.txt"), WordCount, WordCountChecksum),
    };

    public static Func<Options, Measurement>? Find(string name) => Kernels.GetValueOrDefault(name);

    public static string Libraries()
    {
        Type[] types =
        [
            typeof(SKBitmap), typeof(NetVips.Image), typeof(ZstdSharp.Compressor), typeof(BCrypt.Net.BCrypt), typeof(Argon2id),
            typeof(CsvReader), typeof(SylvanCsvReader),
        ];
        return string.Join(", ", types.Select(t => t.Assembly.GetName()).Select(a => $"{a.Name} {Version(a)}"));
    }

    private static string Version(AssemblyName name) =>
        Assembly.Load(name).GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion
            .Split('+')[0] ?? name.Version?.ToString() ?? "?";

    private static string DataFile(Options o, string pattern) =>
        Path.Combine(o.DataDir, string.Format(CultureInfo.InvariantCulture, pattern, o.Size));

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

    private static (int, int, int) SkiaConvert(byte[] data)
    {
        using SKBitmap source = SKBitmap.Decode(data);
        var info = new SKImageInfo(TargetWidth, TargetHeight(source.Width, source.Height));
        using SKBitmap small = source.Resize(info, new SKSamplingOptions(SKCubicResampler.CatmullRom));
        using SKData jpeg = small.Encode(SKEncodedImageFormat.Jpeg, JpegQuality);
        return (small.Width, small.Height, (int)jpeg.Size);
    }

    private static byte[] Corpus(Options o) => File.ReadAllBytes(DataFile(o, "corpus-{0}.bin"));

    private static string PlainChecksum(byte[] raw) =>
        $"{raw.Length}:{Convert.ToHexStringLower(SHA256.HashData(raw))}";

    private static Measurement CompressKernel(Options o, Func<byte[], byte[]> compress, Func<byte[], byte[]> decompress) =>
        KernelTable.MeasureWithExtra(o, () => Corpus(o), compress, packed =>
            new Measurement(PlainChecksum(decompress(packed)), [], new() { ["compressed_bytes"] = packed.Length }));

    private static Measurement DecompressKernel(Options o, Func<byte[], byte[]> compress, Func<byte[], byte[]> decompress) =>
        KernelTable.Measure(o, () => compress(Corpus(o)), decompress, PlainChecksum);

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

    private static string[] Passwords(int n) =>
        Enumerable.Range(0, n).Select(i => $"user-{i}-password").ToArray();

    private static string[] BcryptHashAll(string[] passwords) =>
        passwords.Select(pw => BCrypt.Net.BCrypt.HashPassword(pw, BcryptCost)).ToArray();

    private static string BcryptChecksum(string[] hashes)
    {
        int ok = hashes.Where((h, i) => h[3..7] == $"${BcryptCost}$" && BCrypt.Net.BCrypt.Verify($"user-{i}-password", h))
            .Count();
        int reference = BCrypt.Net.BCrypt.Verify(RefPassword, BcryptRef) ? 1 : 0;
        return $"{hashes.Length}:{ok}:{reference}";
    }

    private static byte[] Argon2Key(string password, byte[] salt, int memory, int time, int lanes, int length)
    {
        using var argon2 = new Argon2id(Encoding.UTF8.GetBytes(password))
        {
            Salt = salt,
            MemorySize = memory,
            Iterations = time,
            DegreeOfParallelism = lanes,
        };
        return argon2.GetBytes(length);
    }

    /// <summary>Konscious only derives the key; the PHC string around it is ours.</summary>
    private static string[] Argon2HashAll(string[] passwords) =>
        passwords.Select(pw =>
        {
            byte[] salt = RandomNumberGenerator.GetBytes(16);
            byte[] key = Argon2Key(pw, salt, Argon2Memory, Argon2Time, Argon2Lanes, 32);
            return Argon2Prefix + B64(salt) + "$" + B64(key);
        }).ToArray();

    private static string B64(byte[] bytes) => Convert.ToBase64String(bytes).TrimEnd('=');

    private static byte[] FromB64(string text) => Convert.FromBase64String(text.PadRight((text.Length + 3) / 4 * 4, '='));

    private static bool Argon2Verifies(string phc, string password)
    {
        string[] parts = phc.Split('$');
        if (parts.Length != 6 || parts[1] != "argon2id" || parts[2] != "v=19")
        {
            return false;
        }
        Dictionary<string, int> p = parts[3].Split(',').Select(kv => kv.Split('='))
            .ToDictionary(kv => kv[0], kv => int.Parse(kv[1], CultureInfo.InvariantCulture));
        byte[] want = FromB64(parts[5]);
        byte[] got = Argon2Key(password, FromB64(parts[4]), p["m"], p["t"], p["p"], want.Length);
        return CryptographicOperations.FixedTimeEquals(got, want);
    }

    private static string Argon2Checksum(string[] hashes)
    {
        int ok = hashes.Where((h, i) => h.StartsWith(Argon2Prefix, StringComparison.Ordinal)
            && Argon2Verifies(h, $"user-{i}-password")).Count();
        int reference = Argon2Verifies(Argon2Ref, RefPassword) ? 1 : 0;
        return $"{hashes.Length}:{ok}:{reference}";
    }

    private sealed record Totals(long Rows, Dictionary<int, long> Sums);

    private static void Add(Dictionary<int, long> sums, int customer, long cents) =>
        sums[customer] = sums.GetValueOrDefault(customer) + cents;

    private static Totals CsvHelperTotals(string path)
    {
        var sums = new Dictionary<int, long>();
        long rows = 0;
        using CsvReader csv = CsvReader.FromFile(path);
        csv.ReadHeader();
        int customer = csv.GetFieldIndex("customer_id"), total = csv.GetFieldIndex("total_cents");
        while (csv.Read())
        {
            rows++;
            Add(sums, csv.Parse<int>(customer), csv.Parse<long>(total));
        }
        return new Totals(rows, sums);
    }

    private static Totals SylvanTotals(string path)
    {
        var sums = new Dictionary<int, long>();
        long rows = 0;
        using SylvanCsvReader csv = SylvanCsvReader.Create(path);
        int customer = csv.GetOrdinal("customer_id"), total = csv.GetOrdinal("total_cents");
        while (csv.Read())
        {
            rows++;
            Add(sums, csv.GetInt32(customer), csv.GetInt64(total));
        }
        return new Totals(rows, sums);
    }

    private static string CsvChecksum(Totals t)
    {
        (int id, long top) = t.Sums.Select(kv => (kv.Key, kv.Value))
            .OrderByDescending(kv => kv.Value).ThenBy(kv => kv.Key).First();
        return $"{t.Rows}:{t.Sums.Count}:{t.Sums.Values.Sum()}:{id}:{top}";
    }

    private static Dictionary<string, long> WordCount(string path)
    {
        var counts = new Dictionary<string, long>();
        foreach (string line in File.ReadLines(path))
        {
            foreach (string word in line.Split(' ', StringSplitOptions.RemoveEmptyEntries))
            {
                counts[word] = counts.GetValueOrDefault(word) + 1;
            }
        }
        return counts;
    }

    private static string WordCountChecksum(Dictionary<string, long> counts)
    {
        (string word, long top) = counts.Select(kv => (kv.Key, kv.Value))
            .OrderByDescending(kv => kv.Value).ThenBy(kv => kv.Key, StringComparer.Ordinal).First();
        return $"{counts.Values.Sum()}:{counts.Count}:{word}:{top}";
    }
}
