// Race: str-find (csharp)
// Extracted from suite/csharp/Algo.cs — build and run via suite/run.py

using System.Globalization;
namespace Kernels;

public static class Algo
{

    private const int PatternLength = 8;

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

    private static (string Text, string Pattern) Text(int n)
    {
        string text = Dna(n, 1)[0];
        return (text, text.Substring(n / 2, PatternLength));
    }

    private static string Matches(int count, int first, int last) => Invariant($"{count}:{first}:{last}");

    public static string StrFind(string text, string pattern)
    {
        int count = 0, first = -1, last = -1;
        int pos = text.IndexOf(pattern, StringComparison.Ordinal);
        while (pos >= 0)
        {
            if (first < 0)
            {
                first = pos;
            }
            last = pos;
            count++;
            pos = text.IndexOf(pattern, pos + 1, StringComparison.Ordinal);
        }
        return Matches(count, first, last);
    }

}
