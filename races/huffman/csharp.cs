// Race: huffman (csharp)
// Extracted from suite/csharp/PuzzleText.cs — build and run via suite/run.py

using System.Globalization;
using System.Numerics;
namespace Kernels;

public sealed record HuffmanResult(int Length, long Bits, byte[] Encoded, bool RoundTrip);

public static class PuzzleText
{

    private const int OneMiB = 1 << 20;

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

}
