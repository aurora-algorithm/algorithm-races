// Race: argon2id (csharp)
// Extracted from suite/csharp/Work.cs — build and run via suite/run.py

using System.Globalization;
using System.IO.Compression;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using CsvHelper;
using Konscious.Security.Cryptography;
using SkiaSharp;
using SylvanCsvReader = Sylvan.Data.Csv.CsvDataReader;
namespace Kernels;

public static class Work
{

    private const int Argon2Memory = 19456;

    private const int Argon2Time = 2;

    private const int Argon2Lanes = 1;

    private const string RefPassword = "correct horse battery staple";

    private const string Argon2Ref =
        "$argon2id$v=19$m=19456,t=2,p=1$c3RhY2tzLWJlbmNoLXNhbHQ$oLeQRnXjHhLMJZcTdYDeVr93DW7srzPjTWhPG3BTryk";

    private static readonly string Argon2Prefix = $"$argon2id$v=19$m={Argon2Memory},t={Argon2Time},p={Argon2Lanes}$";

    private static string[] Passwords(int n) =>
        Enumerable.Range(0, n).Select(i => $"user-{i}-password").ToArray();

    private static byte[] Argon2Key(string password, byte[] salt, int memory, int time, int lanes, int length)
    {
        using var argon2 = new Argon2id(Encoding.UTF8.GetBytes(password))
        {
            Salt = salt,
            MemorySize = memory,
            Iterations = time,
            DegreeOfParallelism = lanes,
        };
        return argon2.GetBytes(length);
    }

    /// <summary>Konscious only derives the key; the PHC string around it is ours.</summary>
    private static string[] Argon2HashAll(string[] passwords) =>
        passwords.Select(pw =>
        {
            byte[] salt = RandomNumberGenerator.GetBytes(16);
            byte[] key = Argon2Key(pw, salt, Argon2Memory, Argon2Time, Argon2Lanes, 32);
            return Argon2Prefix + B64(salt) + "$" + B64(key);
        }).ToArray();

    private static string B64(byte[] bytes) => Convert.ToBase64String(bytes).TrimEnd('=');

    private static byte[] FromB64(string text) => Convert.FromBase64String(text.PadRight((text.Length + 3) / 4 * 4, '='));

    private static bool Argon2Verifies(string phc, string password)
    {
        string[] parts = phc.Split('$');
        if (parts.Length != 6 || parts[1] != "argon2id" || parts[2] != "v=19")
        {
            return false;
        }
        Dictionary<string, int> p = parts[3].Split(',').Select(kv => kv.Split('='))
            .ToDictionary(kv => kv[0], kv => int.Parse(kv[1], CultureInfo.InvariantCulture));
        byte[] want = FromB64(parts[5]);
        byte[] got = Argon2Key(password, FromB64(parts[4]), p["m"], p["t"], p["p"], want.Length);
        return CryptographicOperations.FixedTimeEquals(got, want);
    }

    private static string Argon2Checksum(string[] hashes)
    {
        int ok = hashes.Where((h, i) => h.StartsWith(Argon2Prefix, StringComparison.Ordinal)
            && Argon2Verifies(h, $"user-{i}-password")).Count();
        int reference = Argon2Verifies(Argon2Ref, RefPassword) ? 1 : 0;
        return $"{hashes.Length}:{ok}:{reference}";
    }

}
