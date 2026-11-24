// Race: mandelbrot (csharp)
// Extracted from suite/csharp/BenchmarksGame.cs — build and run via suite/run.py

using System.Globalization;
using System.Text;
namespace Kernels;

public static class BenchmarksGame
{

    public static int RowBytes(int n) => (n + 7) / 8;

    /// <summary>Writes row <paramref name="y"/> of the PBM body into <paramref name="bitmap"/>.</summary>
    public static void MandelbrotRow(int n, int y, byte[] bitmap)
    {
        int offset = y * RowBytes(n);
        double ci = 2.0 * y / n - 1.0;
        for (int x = 0; x < n; x++)
        {
            double cr = 2.0 * x / n - 1.5;
            double zr = 0, zi = 0, tr = 0, ti = 0;
            int i = 0;
            while (i < 50 && tr + ti <= 4.0)
            {
                zi = 2.0 * zr * zi + ci;
                zr = tr - ti + cr;
                tr = zr * zr;
                ti = zi * zi;
                i++;
            }
            if (tr + ti <= 4.0)
            {
                bitmap[offset + (x >> 3)] |= (byte)(0x80 >> (x & 7));
            }
        }
    }

    public static byte[] Mandelbrot(int n)
    {
        var bitmap = new byte[n * RowBytes(n)];
        for (int y = 0; y < n; y++)
        {
            MandelbrotRow(n, y, bitmap);
        }
        return bitmap;
    }

}
