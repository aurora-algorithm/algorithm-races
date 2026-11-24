// Race: regex-log (csharp)
// Extracted from suite/csharp/Misc.cs — build and run via suite/run.py

using System.Globalization;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.RegularExpressions;
namespace Kernels;

public static class Misc
{

    private const string LogPattern = "(GET|POST) /api/v[0-9]/users/([0-9]+) (5[0-9][0-9])";

    /// <summary>Built before the timed region (SPEC section 5: compiled once), the interpreter by default.</summary>
    public static Regex LogRegex() => new(LogPattern);

    public static string RegexLog(Regex regex, string text)
    {
        int count = 0;
        long ids = 0;
        foreach (Match match in regex.Matches(text))
        {
            count++;
            ids += long.Parse(match.Groups[2].Value, CultureInfo.InvariantCulture);
        }
        return string.Create(CultureInfo.InvariantCulture, $"{count}:{ids}");
    }

}
