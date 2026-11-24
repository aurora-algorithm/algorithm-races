using System.Globalization;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace Kernels;

public sealed record Order(long Id, long CustomerId, string Status, long TotalCents, long Items, List<string> Tags);

public sealed record OrderDocument(List<Order> Orders);

public sealed record JsonRoundTrip(int Orders, long TotalCents, long Items, byte[] Output);

public static class Misc
{
    private const string LogPattern = "(GET|POST) /api/v[0-9]/users/([0-9]+) (5[0-9][0-9])";

    /// <summary>Built before the timed region (SPEC section 5: compiled once), the interpreter by default.</summary>
    public static Regex LogRegex() => new(LogPattern);

    public static string RegexLog(Regex regex, string text)
    {
        int count = 0;
        long ids = 0;
        foreach (Match match in regex.Matches(text))
        {
            count++;
            ids += long.Parse(match.Groups[2].Value, CultureInfo.InvariantCulture);
        }
        return string.Create(CultureInfo.InvariantCulture, $"{count}:{ids}");
    }

    public static string HashMap(long[] inserts, long[] lookups)
    {
        var counts = new Dictionary<long, long>();
        foreach (long key in inserts)
        {
            counts[key] = counts.GetValueOrDefault(key) + 1;
        }
        int hits = 0;
        foreach (long key in lookups)
        {
            if (counts.ContainsKey(key))
            {
                hits++;
            }
        }
        return string.Create(CultureInfo.InvariantCulture, $"{counts.Count}:{hits}");
    }

    public static string Sha256(byte[] buffer) => Convert.ToHexStringLower(SHA256.HashData(buffer));

    public static string Sieve(int n)
    {
        var composite = new bool[n + 1];
        for (int i = 2; i * i <= n; i++)
        {
            if (!composite[i])
            {
                for (int j = i * i; j <= n; j += i)
                {
                    composite[j] = true;
                }
            }
        }
        int count = 0;
        for (int i = 2; i <= n; i++)
        {
            if (!composite[i])
            {
                count++;
            }
        }
        return count.ToString(CultureInfo.InvariantCulture);
    }

    public static double[] MatMul(double[] a, double[] b, int n)
    {
        var c = new double[n * n];
        for (int i = 0; i < n; i++)
        {
            int row = i * n;
            for (int j = 0; j < n; j++)
            {
                double sum = 0;
                for (int k = 0; k < n; k++)
                {
                    sum += a[row + k] * b[k * n + j];
                }
                c[row + j] = sum;
            }
        }
        return c;
    }

    public static string MatMulChecksum(double[] c) =>
        string.Create(CultureInfo.InvariantCulture, $"{c.Sum(v => (long)v)}:{Checksum.H31(c)}");

    /// <summary>Parses the file's UTF-8 bytes, as Rust and Go do (a .NET string would be UTF-16).</summary>
    public static JsonRoundTrip Json(byte[] utf8)
    {
        OrderDocument document = JsonSerializer.Deserialize(utf8, KernelsJson.Default.OrderDocument)
            ?? throw new InvalidDataException("orders file holds no document");
        long totalCents = 0, items = 0;
        foreach (Order order in document.Orders)
        {
            totalCents += order.TotalCents;
            items += order.Items;
        }
        byte[] output = JsonSerializer.SerializeToUtf8Bytes(document, KernelsJson.Default.OrderDocument);
        return new JsonRoundTrip(document.Orders.Count, totalCents, items, output);
    }

    public static string JsonChecksum(JsonRoundTrip r) =>
        string.Create(
            CultureInfo.InvariantCulture,
            $"{r.Orders}:{r.TotalCents}:{r.Items}:{r.Output.Length}:{Checksum.Fnv1a(r.Output)}");
}
