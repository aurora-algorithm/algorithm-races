// Race: sieve (csharp)
// Extracted from suite/csharp/Misc.cs — build and run via suite/run.py

using System.Globalization;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.RegularExpressions;
namespace Kernels;

public static class Misc
{

    public static string Sieve(int n)
    {
        var composite = new bool[n + 1];
        for (int i = 2; i * i <= n; i++)
        {
            if (!composite[i])
            {
                for (int j = i * i; j <= n; j += i)
                {
                    composite[j] = true;
                }
            }
        }
        int count = 0;
        for (int i = 2; i <= n; i++)
        {
            if (!composite[i])
            {
                count++;
            }
        }
        return count.ToString(CultureInfo.InvariantCulture);
    }

}
