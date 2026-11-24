// Race: fannkuch (csharp)
// Extracted from suite/csharp/BenchmarksGame.cs — build and run via suite/run.py

using System.Globalization;
using System.Text;
namespace Kernels;

public static class BenchmarksGame
{

    public static string Fannkuch(int n)
    {
        int[] perm1 = Enumerable.Range(0, n).ToArray();
        int[] perm = new int[n];
        int[] count = new int[n];
        int maxFlips = 0, checksum = 0, permCount = 0;
        int r = n;
        while (true)
        {
            while (r != 1)
            {
                count[r - 1] = r;
                r--;
            }
            Array.Copy(perm1, perm, n);
            int flips = 0;
            int k = perm[0];
            while (k != 0)
            {
                Array.Reverse(perm, 0, k + 1);
                flips++;
                k = perm[0];
            }
            maxFlips = Math.Max(maxFlips, flips);
            checksum += permCount % 2 == 0 ? flips : -flips;
            while (true)
            {
                if (r == n)
                {
                    return string.Create(CultureInfo.InvariantCulture, $"{checksum}:{maxFlips}");
                }
                int perm0 = perm1[0];
                Array.Copy(perm1, 1, perm1, 0, r);
                perm1[r] = perm0;
                count[r]--;
                if (count[r] > 0)
                {
                    break;
                }
                r++;
            }
            permCount++;
        }
    }

}
