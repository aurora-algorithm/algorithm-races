using System.Globalization;
using System.Numerics;

namespace Kernels;

public sealed record BwtResult(int Length, int Rounds, int Primary, byte[] Transform);

public sealed record HuffmanResult(int Length, long Bits, byte[] Encoded, bool RoundTrip);

/// <summary>The text and big-number kernels of the "puzzle" group (SPEC.md section 13).</summary>
public static class PuzzleText
{
    private const int OneMiB = 1 << 20;
    private static readonly byte[] Dna = "ACGT"u8.ToArray();

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

    /// <summary>A 1 MiB block of letters skewed toward `a` (two generator values per letter), repeated.</summary>
    internal static byte[] SkewedText(int n)
    {
        var rng = new XorShift32();
        var block = new byte[OneMiB];
        for (int i = 0; i < OneMiB; i++)
        {
            uint a = rng.Next() % 26;
            uint b = rng.Next() % 26;
            block[i] = (byte)(97 + a * b / 26);
        }
        var text = new byte[n / OneMiB * OneMiB];
        for (int offset = 0; offset < text.Length; offset += OneMiB)
        {
            Array.Copy(block, 0, text, offset, OneMiB);
        }
        return text;
    }

    internal static HuffmanResult Huffman(byte[] text)
    {
        var freq = new long[256];
        foreach (byte b in text)
        {
            freq[b]++;
        }
        var heap = new PriorityQueue<long, long>();
        for (int sym = 0; sym < 256; sym++)
        {
            if (freq[sym] > 0)
            {
                long key = freq[sym] * 1024 + sym;
                heap.Enqueue(key, key);
            }
        }
        var left = new int[512];
        var right = new int[512];
        int nextId = 256;
        while (heap.Count > 1)
        {
            long a = heap.Dequeue();
            long b = heap.Dequeue();
            left[nextId] = (int)(a % 1024);
            right[nextId] = (int)(b % 1024);
            long merged = (a / 1024 + b / 1024) * 1024 + nextId;
            heap.Enqueue(merged, merged);
            nextId++;
        }
        int root = (int)(heap.Peek() % 1024);

        var codes = new long[256];
        var lengths = new int[256];
        var stack = new Stack<(int Node, long Code, int Length)>();
        stack.Push((root, 0, 0));
        while (stack.Count > 0)
        {
            (int node, long code, int length) = stack.Pop();
            if (node < 256)
            {
                codes[node] = code;
                lengths[node] = length;
            }
            else
            {
                stack.Push((left[node], code << 1, length + 1));
                stack.Push((right[node], (code << 1) | 1, length + 1));
            }
        }

        var encoded = new List<byte>(text.Length);
        long acc = 0;
        int nbits = 0;
        long bits = 0;
        foreach (byte b in text)
        {
            acc = (acc << lengths[b]) | codes[b];
            nbits += lengths[b];
            bits += lengths[b];
            while (nbits >= 8)
            {
                nbits -= 8;
                encoded.Add((byte)(acc >> nbits));
            }
            acc &= (1L << nbits) - 1;
        }
        if (nbits > 0)
        {
            encoded.Add((byte)(acc << (8 - nbits)));
        }

        var decoded = new byte[text.Length];
        int at = 0;
        int current = root;
        foreach (byte packed in encoded)
        {
            for (int shift = 7; shift >= 0 && at < text.Length; shift--)
            {
                current = ((packed >> shift) & 1) != 0 ? right[current] : left[current];
                if (current < 256)
                {
                    decoded[at++] = (byte)current;
                    current = root;
                }
            }
        }
        bool roundTrip = decoded.AsSpan().SequenceEqual(text);
        return new HuffmanResult(text.Length, bits, encoded.ToArray(), roundTrip);
    }

    internal static string HuffmanChecksum(HuffmanResult r) =>
        Puzzle.Format($"{r.Length}:{r.Bits}:{Checksum.Fnv1a(r.Encoded)}:{(r.RoundTrip ? 1 : 0)}");

    private const long ChudA = 13591409;
    private const long ChudB = 545140134;
    private const long ChudC3Over24 = 10939058860032000;
    private const int DigitsPerTerm = 14;
    private const int GuardDigits = 10;

    /// <summary>P(a,b), Q(a,b), T(a,b) of the Chudnovsky terms a .. b-1.</summary>
    private static (BigInteger P, BigInteger Q, BigInteger T) BinarySplit(long a, long b)
    {
        if (b - a == 1)
        {
            BigInteger p = 1;
            BigInteger q = 1;
            if (a > 0)
            {
                p = new BigInteger(6 * a - 5) * (2 * a - 1) * (6 * a - 1);
                q = new BigInteger(a) * a * a * ChudC3Over24;
            }
            BigInteger t = p * (ChudA + ChudB * a);
            return (p, q, a % 2 == 1 ? -t : t);
        }
        long m = (a + b) / 2;
        (BigInteger p1, BigInteger q1, BigInteger t1) = BinarySplit(a, m);
        (BigInteger p2, BigInteger q2, BigInteger t2) = BinarySplit(m, b);
        return (p1 * p2, q1 * q2, t1 * q2 + p1 * t2);
    }

    /// <summary>Floor square root of n &gt; 0 by precision doubling (CPython's math.isqrt), written out as in the others.</summary>
    private static BigInteger ISqrt(BigInteger n)
    {
        long c = (n.GetBitLength() - 1) / 2;
        BigInteger a = BigInteger.One;
        long d = 0;
        for (int s = 63 - BitOperations.LeadingZeroCount((ulong)c); s >= 0; s--)
        {
            long e = d;
            d = c >> s;
            a = (a << (int)(d - e - 1)) + (n >> (int)(2 * c - e - d + 1)) / a;
        }
        return a * a > n ? a - 1 : a;
    }

    internal static string PiDigits(int n)
    {
        long terms = n / DigitsPerTerm + 2;
        (_, BigInteger q, BigInteger t) = BinarySplit(0, terms);
        BigInteger one = BigInteger.Pow(10, n + GuardDigits);
        BigInteger sqrtC = ISqrt(10005 * one * one);
        BigInteger pi = 426880 * sqrtC * q / t;
        return (pi / BigInteger.Pow(10, GuardDigits)).ToString(CultureInfo.InvariantCulture);
    }

    internal static string PiChecksum(string digits) =>
        Puzzle.Format($"{digits.Length - 1}:{digits[^50..]}:{digits.Sum(c => c - '0')}");
}
