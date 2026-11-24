// Race: palindrome (csharp)
// Extracted from suite/csharp/PuzzleText.cs — build and run via suite/run.py

using System.Globalization;
using System.Numerics;
namespace Kernels;

public static class PuzzleText
{

    /// <summary>`a`/`b` from the bits of generator values, low bit first, 32 characters per value.</summary>
    internal static byte[] BinaryText(int n)
    {
        var rng = new XorShift32();
        var text = new byte[n];
        for (int i = 0; i < n; i += 32)
        {
            uint v = rng.Next();
            for (int j = i; j < Math.Min(i + 32, n); j++)
            {
                text[j] = (byte)((v & 1) != 0 ? 'b' : 'a');
                v >>= 1;
            }
        }
        return text;
    }

    internal static string Palindrome(byte[] text)
    {
        var t = new byte[2 * text.Length + 3];
        Array.Fill(t, (byte)'#');
        t[0] = (byte)'^';
        t[^1] = (byte)'$';
        for (int i = 0; i < text.Length; i++)
        {
            t[2 + 2 * i] = text[i];
        }
        var p = new int[t.Length];
        int center = 0;
        int right = 0;
        for (int i = 1; i < t.Length - 1; i++)
        {
            if (i < right)
            {
                p[i] = Math.Min(right - i, p[2 * center - i]);
            }
            while (t[i + 1 + p[i]] == t[i - 1 - p[i]])
            {
                p[i]++;
            }
            if (i + p[i] > right)
            {
                center = i;
                right = i + p[i];
            }
        }
        int longest = 0;
        int best = 0;
        long count = 0;
        for (int i = 0; i < p.Length; i++)
        {
            count += (p[i] + 1) / 2;
            if (p[i] > longest)
            {
                longest = p[i];
                best = i;
            }
        }
        return Puzzle.Format($"{text.Length}:{longest}:{(best - 1 - longest) / 2}:{count}");
    }

}
