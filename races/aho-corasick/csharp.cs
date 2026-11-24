// Race: aho-corasick (csharp)
// Extracted from suite/csharp/PuzzleText.cs — build and run via suite/run.py

using System.Globalization;
using System.Numerics;
namespace Kernels;

public static class PuzzleText
{

    private const int OneMiB = 1 << 20;

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

    private const int AhoPatterns = 10000;

    internal static (byte[][] Patterns, byte[] Text) AhoInput(int n)
    {
        var rng = new XorShift32();
        var patterns = new byte[AhoPatterns][];
        for (int p = 0; p < AhoPatterns; p++)
        {
            var pattern = new byte[6 + rng.Next() % 7];
            for (int i = 0; i < pattern.Length; i++)
            {
                pattern[i] = Dna[rng.Next() % 4];
            }
            patterns[p] = pattern;
        }
        byte[] block = DnaText(rng, OneMiB);
        var text = new byte[n / OneMiB * OneMiB];
        for (int offset = 0; offset < text.Length; offset += OneMiB)
        {
            Array.Copy(block, 0, text, offset, OneMiB);
        }
        return (patterns, text);
    }

    internal static string AhoCorasick(byte[][] patterns, byte[] text)
    {
        var code = new int[256];
        for (int i = 0; i < Dna.Length; i++)
        {
            code[Dna[i]] = i;
        }
        var trie = new List<int> { -1, -1, -1, -1 };
        var ends = new List<int> { 0 };
        foreach (byte[] pattern in patterns)
        {
            int s = 0;
            foreach (byte ch in pattern)
            {
                int edge = s * 4 + code[ch];
                if (trie[edge] == -1)
                {
                    trie[edge] = ends.Count;
                    trie.AddRange([-1, -1, -1, -1]);
                    ends.Add(0);
                }
                s = trie[edge];
            }
            ends[s]++;
        }
        int[] goTo = trie.ToArray();
        int[] output = ends.ToArray();
        int states = output.Length;
        var fail = new int[states];
        var queue = new int[states];
        int tail = 0;
        for (int c = 0; c < 4; c++)
        {
            if (goTo[c] == -1)
            {
                goTo[c] = 0;
            }
            else
            {
                queue[tail++] = goTo[c];
            }
        }
        for (int head = 0; head < tail; head++)
        {
            int s = queue[head];
            output[s] += output[fail[s]];
            for (int c = 0; c < 4; c++)
            {
                int t = goTo[s * 4 + c];
                if (t == -1)
                {
                    goTo[s * 4 + c] = goTo[fail[s] * 4 + c];
                }
                else
                {
                    fail[t] = goTo[fail[s] * 4 + c];
                    queue[tail++] = t;
                }
            }
        }
        long matches = 0;
        long hits = 0;
        int state = 0;
        foreach (byte ch in text)
        {
            state = goTo[state * 4 + code[ch]];
            int found = output[state];
            if (found > 0)
            {
                matches += found;
                hits++;
            }
        }
        return Puzzle.Format($"{patterns.Length}:{states}:{matches}:{hits}");
    }

}
