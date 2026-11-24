// Race: bcrypt (csharp)
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

    private const int BcryptCost = 10;

    private const string RefPassword = "correct horse battery staple";

    private const string BcryptRef = "$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy";

    private static string[] Passwords(int n) =>
        Enumerable.Range(0, n).Select(i => $"user-{i}-password").ToArray();

    private static string[] BcryptHashAll(string[] passwords) =>
        passwords.Select(pw => BCrypt.Net.BCrypt.HashPassword(pw, BcryptCost)).ToArray();

    private static string BcryptChecksum(string[] hashes)
    {
        int ok = hashes.Where((h, i) => h[3..7] == $"${BcryptCost}$" && BCrypt.Net.BCrypt.Verify($"user-{i}-password", h))
            .Count();
        int reference = BCrypt.Net.BCrypt.Verify(RefPassword, BcryptRef) ? 1 : 0;
        return $"{hashes.Length}:{ok}:{reference}";
    }

}
