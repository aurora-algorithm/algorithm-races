// Race: bwt (csharp)
// Extracted from suite/csharp/PuzzleText.cs — build and run via suite/run.py

using System.Globalization;
using System.Numerics;
namespace Kernels;

public sealed record BwtResult(int Length, int Rounds, int Primary, byte[] Transform);

public static class PuzzleText
{

    private static readonly byte[] Dna = "ACGT"u8.ToArray();

    /// <summary>`ACGT` from 2-bit chunks of generator values, low bits first, 16 characters per value.</summary>
    internal static byte[] DnaText(int n) => DnaText(new XorShift32(), n);

    private static byte[] DnaText(XorShift32 rng, int n)
    {
        var text = new byte[n];
        for (int i = 0; i < n; i += 16)
        {
            uint v = rng.Next();
            for (int j = i; j < Math.Min(i + 16, n); j++)
            {
                text[j] = Dna[v & 3];
                v >>= 2;
            }
        }
        return text;
    }

    internal static BwtResult Bwt(byte[] dna)
    {
        byte[] text = [.. dna, (byte)'$'];
        int n = text.Length;
        int[] rank = text.Select(b => (int)b).ToArray();
        int[] sa = Enumerable.Range(0, n).ToArray();
        var second = new int[n];
        int k = 1;
        int rounds = 0;
        while (true)
        {
            rounds++;
            for (int i = 0; i < n; i++)
            {
                second[i] = i + k < n ? rank[i + k] : -1;
            }
            int[] current = rank;
            Array.Sort(sa, (a, b) =>
                current[a] != current[b] ? current[a].CompareTo(current[b]) : second[a].CompareTo(second[b]));
            var tmp = new int[n];
            for (int j = 1; j < n; j++)
            {
                int a = sa[j - 1];
                int b = sa[j];
                tmp[b] = tmp[a] + (rank[a] != rank[b] || second[a] != second[b] ? 1 : 0);
            }
            rank = tmp;
            if (rank[sa[n - 1]] == n - 1)
            {
                break;
            }
            k *= 2;
        }
        var transform = new byte[n];
        int primary = 0;
        for (int j = 0; j < n; j++)
        {
            if (sa[j] == 0)
            {
                primary = j;
                transform[j] = text[n - 1];
            }
            else
            {
                transform[j] = text[sa[j] - 1];
            }
        }
        return new BwtResult(n, rounds, primary, transform);
    }

    internal static string BwtChecksum(BwtResult r) =>
        Puzzle.Format($"{r.Length}:{r.Rounds}:{r.Primary}:{Checksum.Fnv1a(r.Transform)}");

}
