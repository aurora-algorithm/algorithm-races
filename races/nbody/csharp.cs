// Race: nbody (csharp)
// Extracted from suite/csharp/BenchmarksGame.cs — build and run via suite/run.py

using System.Globalization;
using System.Text;
namespace Kernels;

public sealed class Body(double x, double y, double z, double vx, double vy, double vz, double mass)
{
    public double X = x, Y = y, Z = z, Vx = vx, Vy = vy, Vz = vz;
    public readonly double Mass = mass;
}

public static class BenchmarksGame
{

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

}
