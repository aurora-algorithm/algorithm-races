// Race: hyperloglog (csharp)
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

    private const int HllP = 14;

    private const int HllM = 1 << HllP;

    private static string HyperLogLog(int n)
    {
        var rng = new XorShift32();
        uint universe = (uint)Math.Max(n / 4, 1);
        var registers = new byte[HllM];
        for (int k = 0; k < n; k++)
        {
            uint h = Fmix32(rng.Next() % universe);
            uint w = h << HllP;
            int rank = w == 0 ? 32 - HllP + 1 : BitOperations.LeadingZeroCount(w) + 1;
            uint idx = h >> (32 - HllP);
            if (rank > registers[idx])
            {
                registers[idx] = (byte)rank;
            }
        }
        double z = 0;
        int zeros = 0;
        foreach (byte r in registers)
        {
            z += Math.ScaleB(1.0, -r);
            if (r == 0)
            {
                zeros++;
            }
        }
        double m = HllM;
        double estimate = 0.7213 / (1.0 + 1.079 / m) * m * m / z;
        if (estimate <= 2.5 * m && zeros > 0)
        {
            estimate = m * Math.Log(m / zeros);
        }
        long rounded = (long)Math.Floor(estimate + 0.5);
        uint hash = Checksum.H31(registers.Select(r => (long)r));
        return Format($"{n}:{rounded}:{hash}");
    }

}
