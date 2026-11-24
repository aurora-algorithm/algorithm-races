using System.Buffers.Binary;
using System.Globalization;
using System.Text;

namespace Kernels;

/// <summary>Marsaglia's xorshift32; every input gets a fresh generator with the shared seed.</summary>
public sealed class XorShift32(uint seed = 2463534242)
{
    private uint _state = seed;

    public uint Next()
    {
        _state ^= _state << 13;
        _state ^= _state >> 17;
        _state ^= _state << 5;
        return _state;
    }

    public long[] Ints(int n)
    {
        var values = new long[n];
        for (int i = 0; i < n; i++)
        {
            values[i] = Next();
        }
        return values;
    }
}

public static class Checksum
{
    public static uint H31(IEnumerable<long> values)
    {
        uint h = 0;
        foreach (long v in values)
        {
            h = h * 31 + (uint)v;
        }
        return h;
    }

    public static uint H31(double[] values) => H31(values.Select(v => (long)v));

    public static uint Fnv1a(byte[] data)
    {
        uint h = 2166136261;
        foreach (byte b in data)
        {
            h = (h ^ b) * 16777619;
        }
        return h;
    }

    public static uint Fnv1a(string text) => Fnv1a(Encoding.UTF8.GetBytes(text));
}

public static class Inputs
{
    private static readonly string[] Methods = ["GET", "POST", "PUT", "DELETE"];
    private static readonly int[] Statuses = [200, 201, 204, 301, 304, 400, 401, 403, 404, 500, 502, 503];
    private const int OneMiB = 1 << 20;

    public static long[] Ints(int n) => new XorShift32().Ints(n);

    /// <summary>All 64 bits used: two draws per value, high word first, read as a signed long.</summary>
    public static long[] Ints64(int n)
    {
        var rng = new XorShift32();
        var values = new long[n];
        for (int i = 0; i < n; i++)
        {
            ulong hi = rng.Next();
            ulong lo = rng.Next();
            values[i] = unchecked((long)((hi << 32) | lo));
        }
        return values;
    }

    public static string[] Strings(int n)
    {
        var rng = new XorShift32();
        var strings = new string[n];
        for (int i = 0; i < n; i++)
        {
            var chars = new char[8 + rng.Next() % 9];
            for (int c = 0; c < chars.Length; c++)
            {
                chars[c] = (char)('a' + rng.Next() % 26);
            }
            strings[i] = new string(chars);
        }
        return strings;
    }

    public static byte[] ShaBuffer(int size)
    {
        var rng = new XorShift32();
        var block = new byte[OneMiB];
        for (int i = 0; i < OneMiB; i += 4)
        {
            BinaryPrimitives.WriteUInt32LittleEndian(block.AsSpan(i), rng.Next());
        }
        var buffer = new byte[size];
        for (int offset = 0; offset < size; offset += OneMiB)
        {
            Array.Copy(block, 0, buffer, offset, OneMiB);
        }
        return buffer;
    }

    public static string Log(int lines)
    {
        var rng = new XorShift32();
        var text = new StringBuilder();
        for (int i = 0; i < lines; i++)
        {
            string method = Methods[rng.Next() % 4];
            uint version = 1 + rng.Next() % 3;
            uint user = rng.Next() % 100000;
            int status = Statuses[rng.Next() % 12];
            uint ms = rng.Next() % 1000;
            text.Append(CultureInfo.InvariantCulture, $"{method} /api/v{version}/users/{user} {status} {ms}ms\n");
        }
        return text.ToString();
    }

    public static (double[] A, double[] B) Matrices(int n)
    {
        var rng = new XorShift32();
        double[] a = rng.Ints(n * n).Select(v => (double)(v % 10)).ToArray();
        double[] b = rng.Ints(n * n).Select(v => (double)(v % 10)).ToArray();
        return (a, b);
    }

    public static (long[] Inserts, long[] Lookups) HashMap(int n)
    {
        long[] inserts = Ints(n);
        long[] lookups = inserts.Select((key, i) => key ^ (i & 1)).ToArray();
        return (inserts, lookups);
    }

    public static byte[] Orders(int n, string dataDir) =>
        File.ReadAllBytes(Path.Combine(dataDir, $"orders-{n}.json"));
}
