// Race: sha256 (csharp)
// Extracted from suite/csharp/Misc.cs — build and run via suite/run.py

using System.Globalization;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.RegularExpressions;
namespace Kernels;

public static class Misc
{

    public static string Sha256(byte[] buffer) => Convert.ToHexStringLower(SHA256.HashData(buffer));

}
