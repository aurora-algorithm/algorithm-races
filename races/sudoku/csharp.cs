// Race: sudoku (csharp)
// Extracted from suite/csharp/Puzzle.cs — build and run via suite/run.py

using System.Globalization;
using System.Numerics;
using static Kernels.PuzzleText;
namespace Kernels;

public static class Puzzle
{

    internal static string Format(FormattableString text) => text.ToString(CultureInfo.InvariantCulture);

    private static void Shuffle(XorShift32 rng, int[] items)
    {
        for (int i = items.Length - 1; i > 0; i--)
        {
            int j = (int)(rng.Next() % (uint)(i + 1));
            (items[i], items[j]) = (items[j], items[i]);
        }
    }

    private const int SudokuClues = 26;

    private static int[][] SudokuPuzzles(int n)
    {
        var rng = new XorShift32();
        var puzzles = new int[n][];
        for (int p = 0; p < n; p++)
        {
            int[] digits = [1, 2, 3, 4, 5, 6, 7, 8, 9];
            Shuffle(rng, digits);
            int[] rows = ShuffledLines(rng);
            int[] cols = ShuffledLines(rng);
            int[] cells = Enumerable.Range(0, 81).ToArray();
            Shuffle(rng, cells);
            var puzzle = new int[81];
            foreach (int cell in cells.Take(SudokuClues))
            {
                int r = rows[cell / 9];
                int c = cols[cell % 9];
                puzzle[cell] = digits[(r * 3 + r / 3 + c) % 9];
            }
            puzzles[p] = puzzle;
        }
        return puzzles;
    }

    private static int[] ShuffledLines(XorShift32 rng)
    {
        int[] bands = [0, 1, 2];
        Shuffle(rng, bands);
        var lines = new List<int>(9);
        foreach (int band in bands)
        {
            int[] inner = [0, 1, 2];
            Shuffle(rng, inner);
            lines.AddRange(inner.Select(i => band * 3 + i));
        }
        return lines.ToArray();
    }

    /// <summary>The exact-cover matrix as flat arrays: node 0 is the root, 1..324 the column headers, then 4 nodes
    /// per row.</summary>
    private sealed class Dlx
    {
        public const int Columns = 324;

        private readonly int[] _left;
        private readonly int[] _right;
        private readonly int[] _up;
        private readonly int[] _down;
        private readonly int[] _count = new int[Columns + 1];

        public Dlx()
        {
            int size = 1 + Columns + 729 * 4;
            _left = new int[size];
            _right = new int[size];
            _up = Enumerable.Range(0, size).ToArray();
            _down = Enumerable.Range(0, size).ToArray();
            Col = Enumerable.Range(0, size).ToArray();
            Row = new int[size];
            for (int i = 0; i <= Columns; i++)
            {
                _left[i] = i - 1;
                _right[i] = i + 1;
            }
            _left[0] = Columns;
            _right[Columns] = 0;
            int node = Columns + 1;
            for (int r = 0; r < 9; r++)
            {
                for (int c = 0; c < 9; c++)
                {
                    int box = r / 3 * 3 + c / 3;
                    for (int d = 0; d < 9; d++)
                    {
                        int first = node;
                        int[] columns = [1 + r * 9 + c, 82 + r * 9 + d, 163 + c * 9 + d, 244 + box * 9 + d];
                        foreach (int column in columns)
                        {
                            Col[node] = column;
                            Row[node] = (r * 9 + c) * 9 + d;
                            _up[node] = _up[column];
                            _down[node] = column;
                            _down[_up[column]] = node;
                            _up[column] = node;
                            _count[column]++;
                            _left[node] = node - 1;
                            _right[node] = node + 1;
                            node++;
                        }
                        _left[first] = node - 1;
                        _right[node - 1] = first;
                    }
                }
            }
        }

        public int[] Col { get; }

        public int[] Row { get; }

        public int[] Chosen { get; } = new int[81];

        public int[] First { get; private set; } = [];

        public long Solutions { get; private set; }

        public long Tries { get; private set; }

        public void Cover(int c)
        {
            _right[_left[c]] = _right[c];
            _left[_right[c]] = _left[c];
            for (int i = _down[c]; i != c; i = _down[i])
            {
                for (int j = _right[i]; j != i; j = _right[j])
                {
                    _down[_up[j]] = _down[j];
                    _up[_down[j]] = _up[j];
                    _count[Col[j]]--;
                }
            }
        }

        public void Uncover(int c)
        {
            for (int i = _up[c]; i != c; i = _up[i])
            {
                for (int j = _left[i]; j != i; j = _left[j])
                {
                    _count[Col[j]]++;
                    _down[_up[j]] = j;
                    _up[_down[j]] = j;
                }
            }
            _right[_left[c]] = c;
            _left[_right[c]] = c;
        }

        public void ChooseRow(int node)
        {
            for (int j = _right[node]; j != node; j = _right[j])
            {
                Cover(Col[j]);
            }
        }

        private void UnchooseRow(int node)
        {
            for (int j = _left[node]; j != node; j = _left[j])
            {
                Uncover(Col[j]);
            }
        }

        public void Search(int depth)
        {
            if (_right[0] == 0)
            {
                Solutions++;
                if (Solutions == 1)
                {
                    First = Chosen[..depth];
                }
                return;
            }
            int best = _right[0];
            for (int c = _right[best]; c != 0; c = _right[c])
            {
                if (_count[c] < _count[best])
                {
                    best = c;
                }
            }
            Cover(best);
            for (int r = _down[best]; r != best; r = _down[r])
            {
                Tries++;
                Chosen[depth] = Row[r];
                ChooseRow(r);
                Search(depth + 1);
                UnchooseRow(r);
            }
            Uncover(best);
        }
    }

    private static string Sudoku(int[][] puzzles)
    {
        long solutions = 0;
        long tries = 0;
        uint h = 0;
        foreach (int[] puzzle in puzzles)
        {
            var dlx = new Dlx();
            int depth = 0;
            for (int cell = 0; cell < 81; cell++)
            {
                int digit = puzzle[cell];
                if (digit != 0)
                {
                    int node = Dlx.Columns + 1 + (cell * 9 + digit - 1) * 4;
                    dlx.Cover(dlx.Col[node]);
                    dlx.ChooseRow(node);
                    dlx.Chosen[depth] = cell * 9 + digit - 1;
                    depth++;
                }
            }
            dlx.Search(depth);
            solutions += dlx.Solutions;
            tries += dlx.Tries;
            var grid = new int[81];
            foreach (int row in dlx.First)
            {
                grid[row / 9] = row % 9 + 1;
            }
            foreach (int digit in grid)
            {
                h = h * 31 + (uint)digit;
            }
        }
        return Format($"{puzzles.Length}:{solutions}:{tries}:{h}");
    }

}
