// Race: edit-distance (csharp)
// Extracted from suite/csharp/Algo.cs — build and run via suite/run.py

using System.Globalization;
namespace Kernels;

public static class Algo
{

    private const string Acgt = "acgt";

    internal static string Invariant(FormattableString text) => text.ToString(CultureInfo.InvariantCulture);

    private static string Same(string s) => s;

    private static string[] Dna(int length, int count)
    {
        var rng = new XorShift32();
        var strings = new string[count];
        for (int s = 0; s < count; s++)
        {
            var chars = new char[length];
            for (int i = 0; i < length; i++)
            {
                chars[i] = Acgt[(int)(rng.Next() % 4)];
            }
            strings[s] = new string(chars);
        }
        return strings;
    }

    public static string EditDistance(string a, string b)
    {
        var prev = new int[b.Length + 1];
        var cur = new int[b.Length + 1];
        for (int j = 0; j <= b.Length; j++)
        {
            prev[j] = j;
        }
        for (int i = 0; i < a.Length; i++)
        {
            cur[0] = i + 1;
            for (int j = 0; j < b.Length; j++)
            {
                int cost = a[i] == b[j] ? 0 : 1;
                cur[j + 1] = Math.Min(Math.Min(prev[j + 1] + 1, cur[j] + 1), prev[j] + cost);
            }
            (prev, cur) = (cur, prev);
        }
        return Invariant($"{prev[b.Length]}");
    }

}
