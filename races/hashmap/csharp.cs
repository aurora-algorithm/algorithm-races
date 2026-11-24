// Race: hashmap (csharp)
// Extracted from suite/csharp/Misc.cs — build and run via suite/run.py

using System.Globalization;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.RegularExpressions;
namespace Kernels;

public static class Misc
{

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

}
