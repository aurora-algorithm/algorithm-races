// Race: kmp (csharp)
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

    public static string Kmp(string text, string pattern)
    {
        int m = pattern.Length;
        var fail = new int[m];
        int k = 0;
        for (int i = 1; i < m; i++)
        {
            while (k > 0 && pattern[i] != pattern[k])
            {
                k = fail[k - 1];
            }
            if (pattern[i] == pattern[k])
            {
                k++;
            }
            fail[i] = k;
        }
        int count = 0, first = -1, last = -1;
        k = 0;
        for (int i = 0; i < text.Length; i++)
        {
            char ch = text[i];
            while (k > 0 && ch != pattern[k])
            {
                k = fail[k - 1];
            }
            if (ch == pattern[k])
            {
                k++;
            }
            if (k == m)
            {
                last = i + 1 - m;
                if (first < 0)
                {
                    first = last;
                }
                count++;
                k = fail[k - 1];
            }
        }
        return Matches(count, first, last);
    }

}
