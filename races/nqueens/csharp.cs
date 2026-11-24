// Race: nqueens (csharp)
// Extracted from suite/csharp/Algo.cs — build and run via suite/run.py

using System.Globalization;
namespace Kernels;

public static class Algo
{

    internal static string Invariant(FormattableString text) => text.ToString(CultureInfo.InvariantCulture);

    private static string Same(string s) => s;

    public static long NQueens(int n)
    {
        var cols = new bool[n];
        var up = new bool[2 * n - 1];
        var down = new bool[2 * n - 1];
        return Place(0);

        long Place(int row)
        {
            if (row == n)
            {
                return 1;
            }
            long count = 0;
            for (int col = 0; col < n; col++)
            {
                int u = row + col, d = row + n - 1 - col;
                if (!cols[col] && !up[u] && !down[d])
                {
                    cols[col] = up[u] = down[d] = true;
                    count += Place(row + 1);
                    cols[col] = up[u] = down[d] = false;
                }
            }
            return count;
        }
    }

}
