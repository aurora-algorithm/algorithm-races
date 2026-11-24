// Race: matmul (csharp)
// Extracted from suite/csharp/Misc.cs — build and run via suite/run.py

using System.Globalization;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.RegularExpressions;
namespace Kernels;

public static class Misc
{

    public static double[] MatMul(double[] a, double[] b, int n)
    {
        var c = new double[n * n];
        for (int i = 0; i < n; i++)
        {
            int row = i * n;
            for (int j = 0; j < n; j++)
            {
                double sum = 0;
                for (int k = 0; k < n; k++)
                {
                    sum += a[row + k] * b[k * n + j];
                }
                c[row + j] = sum;
            }
        }
        return c;
    }

    public static string MatMulChecksum(double[] c) =>
        string.Create(CultureInfo.InvariantCulture, $"{c.Sum(v => (long)v)}:{Checksum.H31(c)}");

}
