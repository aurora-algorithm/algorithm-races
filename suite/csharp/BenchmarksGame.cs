using System.Globalization;
using System.Text;

namespace Kernels;

public sealed class Node(Node? left, Node? right)
{
    public static Node Make(int depth) =>
        depth == 0 ? new Node(null, null) : new Node(Make(depth - 1), Make(depth - 1));

    public int Check() => left is null || right is null ? 1 : 1 + left.Check() + right.Check();
}

public sealed class Body(double x, double y, double z, double vx, double vy, double vz, double mass)
{
    public double X = x, Y = y, Z = z, Vx = vx, Vy = vy, Vz = vz;
    public readonly double Mass = mass;
}

public static class BenchmarksGame
{
    internal const int MinDepth = 4;

    public static int DepthChecks(int depth, int iterations)
    {
        int check = 0;
        for (int i = 0; i < iterations; i++)
        {
            check += Node.Make(depth).Check();
        }
        return check;
    }

    public static string BinaryTrees(int n) => BinaryTreesLines(n, DepthChecks);

    /// <summary>The Benchmarks Game output text; <paramref name="depthRun"/> sums the checks for one depth.</summary>
    public static string BinaryTreesLines(int n, Func<int, int, int> depthRun)
    {
        int maxDepth = Math.Max(MinDepth + 2, n);
        int stretch = maxDepth + 1;
        var output = new StringBuilder();
        var culture = CultureInfo.InvariantCulture;
        output.Append(culture, $"stretch tree of depth {stretch}\t check: {Node.Make(stretch).Check()}\n");
        Node longLived = Node.Make(maxDepth);
        for (int depth = MinDepth; depth <= maxDepth; depth += 2)
        {
            int iterations = 1 << (maxDepth - depth + MinDepth);
            output.Append(culture, $"{iterations}\t trees of depth {depth}\t check: {depthRun(depth, iterations)}\n");
        }
        output.Append(culture, $"long lived tree of depth {maxDepth}\t check: {longLived.Check()}\n");
        return output.ToString();
    }

    private const double Pi = 3.141592653589793;
    private const double SolarMass = 4 * Pi * Pi;
    private const double DaysPerYear = 365.24;

    private static Body[] CreateBodies() =>
    [
        new(0, 0, 0, 0, 0, 0, SolarMass),
        new(
            4.84143144246472090e+00,
            -1.16032004402742839e+00,
            -1.03622044471123109e-01,
            1.66007664274403694e-03 * DaysPerYear,
            7.69901118419740425e-03 * DaysPerYear,
            -6.90460016972063023e-05 * DaysPerYear,
            9.54791938424326609e-04 * SolarMass),
        new(
            8.34336671824457987e+00,
            4.12479856412430479e+00,
            -4.03523417114321381e-01,
            -2.76742510726862411e-03 * DaysPerYear,
            4.99852801234917238e-03 * DaysPerYear,
            2.30417297573763929e-05 * DaysPerYear,
            2.85885980666130812e-04 * SolarMass),
        new(
            1.28943695621391310e+01,
            -1.51111514016986312e+01,
            -2.23307578892655734e-01,
            2.96460137564761618e-03 * DaysPerYear,
            2.37847173959480950e-03 * DaysPerYear,
            -2.96589568540237556e-05 * DaysPerYear,
            4.36624404335156298e-05 * SolarMass),
        new(
            1.53796971148509165e+01,
            -2.59193146099879641e+01,
            1.79258772950371181e-01,
            2.68067772490389322e-03 * DaysPerYear,
            1.62824170038242295e-03 * DaysPerYear,
            -9.51592254519715870e-05 * DaysPerYear,
            5.15138902046611451e-05 * SolarMass),
    ];

    private static void OffsetMomentum(Body[] bodies)
    {
        double px = 0, py = 0, pz = 0;
        foreach (Body b in bodies)
        {
            px += b.Vx * b.Mass;
            py += b.Vy * b.Mass;
            pz += b.Vz * b.Mass;
        }
        Body sun = bodies[0];
        sun.Vx = -px / SolarMass;
        sun.Vy = -py / SolarMass;
        sun.Vz = -pz / SolarMass;
    }

    private static double Energy(Body[] bodies)
    {
        double e = 0;
        for (int i = 0; i < bodies.Length; i++)
        {
            Body bi = bodies[i];
            e += 0.5 * bi.Mass * (bi.Vx * bi.Vx + bi.Vy * bi.Vy + bi.Vz * bi.Vz);
            for (int j = i + 1; j < bodies.Length; j++)
            {
                Body bj = bodies[j];
                double dx = bi.X - bj.X;
                double dy = bi.Y - bj.Y;
                double dz = bi.Z - bj.Z;
                e -= bi.Mass * bj.Mass / Math.Sqrt(dx * dx + dy * dy + dz * dz);
            }
        }
        return e;
    }

    private static void Advance(Body[] bodies, double dt, int steps)
    {
        for (int step = 0; step < steps; step++)
        {
            for (int i = 0; i < bodies.Length; i++)
            {
                Body bi = bodies[i];
                for (int j = i + 1; j < bodies.Length; j++)
                {
                    Body bj = bodies[j];
                    double dx = bi.X - bj.X;
                    double dy = bi.Y - bj.Y;
                    double dz = bi.Z - bj.Z;
                    double d2 = dx * dx + dy * dy + dz * dz;
                    double mag = dt / (d2 * Math.Sqrt(d2));
                    double bim = bi.Mass * mag;
                    double bjm = bj.Mass * mag;
                    bi.Vx -= dx * bjm;
                    bi.Vy -= dy * bjm;
                    bi.Vz -= dz * bjm;
                    bj.Vx += dx * bim;
                    bj.Vy += dy * bim;
                    bj.Vz += dz * bim;
                }
            }
            foreach (Body b in bodies)
            {
                b.X += dt * b.Vx;
                b.Y += dt * b.Vy;
                b.Z += dt * b.Vz;
            }
        }
    }

    public static string NBody(int steps)
    {
        Body[] bodies = CreateBodies();
        OffsetMomentum(bodies);
        double before = Energy(bodies);
        Advance(bodies, 0.01, steps);
        double after = Energy(bodies);
        return string.Create(CultureInfo.InvariantCulture, $"{before:F9};{after:F9}");
    }

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
