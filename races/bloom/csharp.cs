// Race: bloom (csharp)
// Extracted from suite/csharp/Puzzle.cs — build and run via suite/run.py

using System.Globalization;
using System.Numerics;
using static Kernels.PuzzleText;
namespace Kernels;

public static class Puzzle
{

    internal static string Format(FormattableString text) => text.ToString(CultureInfo.InvariantCulture);

    /// <summary>MurmurHash3's 32-bit finalizer: the one hash of the probabilistic kernels.</summary>
    private static uint Fmix32(uint h)
    {
        h ^= h >> 16;
        h *= 0x85ebca6b;
        h ^= h >> 13;
        h *= 0xc2b2ae35;
        h ^= h >> 16;
        return h;
    }

    private const int BloomBitsPerKey = 10;

    private const int BloomHashes = 7;

    private const uint Golden = 0x9e3779b9;

    private static (long[] Keys, long[] Lookups) BloomInput(int n)
    {
        long[] values = Inputs.Ints(2 * n);
        return (values[..n], values[n..]);
    }

    private static string Bloom(long[] keys, long[] lookups)
    {
        uint m = (uint)(BloomBitsPerKey * keys.Length);
        var bits = new byte[(m + 7) / 8];
        foreach (long key in keys)
        {
            uint h1 = Fmix32((uint)key);
            uint h2 = Fmix32((uint)key ^ Golden) | 1;
            for (uint i = 0; i < BloomHashes; i++)
            {
                uint b = (h1 + i * h2) % m;
                bits[b >> 3] |= (byte)(1 << (int)(b & 7));
            }
        }
        int falsePositives = 0;
        foreach (long key in lookups)
        {
            uint h1 = Fmix32((uint)key);
            uint h2 = Fmix32((uint)key ^ Golden) | 1;
            bool all = true;
            for (uint i = 0; i < BloomHashes; i++)
            {
                uint b = (h1 + i * h2) % m;
                if ((bits[b >> 3] & (1 << (int)(b & 7))) == 0)
                {
                    all = false;
                    break;
                }
            }
            if (all)
            {
                falsePositives++;
            }
        }
        long setBits = bits.Sum(b => (long)BitOperations.PopCount(b));
        return Format($"{keys.Length}:{setBits}:{falsePositives}");
    }

}
