// Race: spectral-norm (csharp)
// Extracted from suite/csharp/BenchmarksGame.cs — build and run via suite/run.py

using System.Globalization;
using System.Text;
namespace Kernels;

public static class BenchmarksGame
{

    private static double A(long i, long j) => 1.0 / ((i + j) * (i + j + 1) / 2 + i + 1);

    private static void MultiplyAv(double[] u, double[] result)
    {
        for (int i = 0; i < u.Length; i++)
        {
            double sum = 0;
            for (int j = 0; j < u.Length; j++)
            {
                sum += A(i, j) * u[j];
            }
            result[i] = sum;
        }
    }

    private static void MultiplyAtv(double[] u, double[] result)
    {
        for (int i = 0; i < u.Length; i++)
        {
            double sum = 0;
            for (int j = 0; j < u.Length; j++)
            {
                sum += A(j, i) * u[j];
            }
            result[i] = sum;
        }
    }

    private static void MultiplyAtAv(double[] u, double[] result, double[] temp)
    {
        MultiplyAv(u, temp);
        MultiplyAtv(temp, result);
    }

    public static string SpectralNorm(int n)
    {
        double[] u = Enumerable.Repeat(1.0, n).ToArray();
        double[] v = new double[n];
        double[] temp = new double[n];
        for (int i = 0; i < 10; i++)
        {
            MultiplyAtAv(u, v, temp);
            MultiplyAtAv(v, u, temp);
        }
        double vBv = 0, vv = 0;
        for (int i = 0; i < n; i++)
        {
            vBv += u[i] * v[i];
            vv += v[i] * v[i];
        }
        return Math.Sqrt(vBv / vv).ToString("F9", CultureInfo.InvariantCulture);
    }

}
