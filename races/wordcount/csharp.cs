// Race: wordcount (csharp)
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
