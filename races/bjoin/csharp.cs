// Race: bjoin.go.strings.Join (csharp)
// Extracted from suite/csharp/WaysMaps.cs — build and run via suite/run.py

using System.Collections.Concurrent;
using System.Collections.Frozen;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Text;
namespace Kernels;

public static class WaysMaps
{

    private static Measurement Join(Options o, Func<string[], string> join) =>
        KernelTable.Measure(o, () => Inputs.Ints(o.Size).Select(v => v.ToString(CultureInfo.InvariantCulture)).ToArray(),
            join, Joined);

    private static string BuildWithCapacity(string[] parts)
    {
        int total = Math.Max(parts.Length - 1, 0);
        foreach (string p in parts)
        {
            total += p.Length;
        }
        return Build(new StringBuilder(total), parts);
    }

    private static string Joined(string s) =>
        $"{Encoding.UTF8.GetByteCount(s)}:{Checksum.Fnv1a(s).ToString(CultureInfo.InvariantCulture)}";

}
