// Race: csv (csharp)
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

    private sealed record Totals(long Rows, Dictionary<int, long> Sums);

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

    private static string CsvChecksum(Totals t)
    {
        (int id, long top) = t.Sums.Select(kv => (kv.Key, kv.Value))
            .OrderByDescending(kv => kv.Value).ThenBy(kv => kv.Key).First();
        return $"{t.Rows}:{t.Sums.Count}:{t.Sums.Values.Sum()}:{id}:{top}";
    }

}
