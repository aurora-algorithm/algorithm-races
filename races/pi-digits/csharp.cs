// Race: pi-digits (csharp)
// Extracted from suite/csharp/PuzzleText.cs — build and run via suite/run.py

using System.Globalization;
using System.Numerics;
namespace Kernels;

public static class PuzzleText
{

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
