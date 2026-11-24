using System.Globalization;
using System.Numerics;
using static Kernels.PuzzleText;

namespace Kernels;

public sealed record SkipResult(SkipNode Head, int Count, int Hits, long Heights);

public sealed class SkipNode(long key, int height)
{
    public long Key { get; } = key;

    public SkipNode?[] Next { get; } = new SkipNode?[height];
}

/// <summary>The "puzzle" kernels (kernels/SPEC.md section 13): little-known algorithms, one trick each.</summary>
public static class Puzzle
{
    private static readonly Dictionary<string, Func<Options, Measurement>> Kernels = new()
    {
        ["sudoku"] = o => KernelTable.Measure(o, () => SudokuPuzzles(o.Size), Sudoku, s => s),
        ["bloom"] = o => KernelTable.Measure(o, () => BloomInput(o.Size), kv => Bloom(kv.Keys, kv.Lookups), s => s),
        ["hyperloglog"] = o => KernelTable.Measure(o, () => o.Size, HyperLogLog, s => s),
        ["skiplist"] = o =>
            KernelTable.Measure(o, () => Inputs.HashMap(o.Size), kv => SkipList(kv.Inserts, kv.Lookups), SkipChecksum),
        ["cycle"] = o => KernelTable.Measure(o, () => CycleStarts(o.Size), Cycle, s => s),
        ["palindrome"] = o => KernelTable.Measure(o, () => BinaryText(o.Size), Palindrome, s => s),
        ["bwt"] = o => KernelTable.Measure(o, () => DnaText(o.Size), Bwt, BwtChecksum),
        ["aho-corasick"] = o =>
            KernelTable.Measure(o, () => AhoInput(o.Size), d => AhoCorasick(d.Patterns, d.Text), s => s),
        ["pi-digits"] = o => KernelTable.Measure(o, () => o.Size, PiDigits, PiChecksum),
        ["life"] = o => KernelTable.Measure(o, LifeInput, grid => Life(grid, o.Size), g => LifeChecksum(o.Size, g)),
        ["huffman"] = o => KernelTable.Measure(o, () => SkewedText(o.Size), Huffman, HuffmanChecksum),
        ["window-max"] = o => KernelTable.Measure(o, () => Inputs.Ints(o.Size), WindowMax, s => s),
        ["rain-water"] = o => KernelTable.Measure(o, () => Terrain(o.Size), RainWater, s => s),
        ["collatz"] = o => KernelTable.Measure(o, () => o.Size, Collatz, s => s),
    };

    public static Func<Options, Measurement>? Find(string name) => Kernels.GetValueOrDefault(name);

    internal static string Format(FormattableString text) => text.ToString(CultureInfo.InvariantCulture);

    /// <summary>MurmurHash3's 32-bit finalizer: the one hash of the probabilistic kernels.</summary>
    private static uint Fmix32(uint h)
    {
        h ^= h >> 16;
        h *= 0x85ebca6b;
        h ^= h >> 13;
        h *= 0xc2b2ae35;
        h ^= h >> 16;
        return h;
    }

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

    private const int BloomBitsPerKey = 10;
    private const int BloomHashes = 7;
    private const uint Golden = 0x9e3779b9;

    private static (long[] Keys, long[] Lookups) BloomInput(int n)
    {
        long[] values = Inputs.Ints(2 * n);
        return (values[..n], values[n..]);
    }

    private static string Bloom(long[] keys, long[] lookups)
    {
        uint m = (uint)(BloomBitsPerKey * keys.Length);
        var bits = new byte[(m + 7) / 8];
        foreach (long key in keys)
        {
            uint h1 = Fmix32((uint)key);
            uint h2 = Fmix32((uint)key ^ Golden) | 1;
            for (uint i = 0; i < BloomHashes; i++)
            {
                uint b = (h1 + i * h2) % m;
                bits[b >> 3] |= (byte)(1 << (int)(b & 7));
            }
        }
        int falsePositives = 0;
        foreach (long key in lookups)
        {
            uint h1 = Fmix32((uint)key);
            uint h2 = Fmix32((uint)key ^ Golden) | 1;
            bool all = true;
            for (uint i = 0; i < BloomHashes; i++)
            {
                uint b = (h1 + i * h2) % m;
                if ((bits[b >> 3] & (1 << (int)(b & 7))) == 0)
                {
                    all = false;
                    break;
                }
            }
            if (all)
            {
                falsePositives++;
            }
        }
        long setBits = bits.Sum(b => (long)BitOperations.PopCount(b));
        return Format($"{keys.Length}:{setBits}:{falsePositives}");
    }

    private const int HllP = 14;
    private const int HllM = 1 << HllP;

    private static string HyperLogLog(int n)
    {
        var rng = new XorShift32();
        uint universe = (uint)Math.Max(n / 4, 1);
        var registers = new byte[HllM];
        for (int k = 0; k < n; k++)
        {
            uint h = Fmix32(rng.Next() % universe);
            uint w = h << HllP;
            int rank = w == 0 ? 32 - HllP + 1 : BitOperations.LeadingZeroCount(w) + 1;
            uint idx = h >> (32 - HllP);
            if (rank > registers[idx])
            {
                registers[idx] = (byte)rank;
            }
        }
        double z = 0;
        int zeros = 0;
        foreach (byte r in registers)
        {
            z += Math.ScaleB(1.0, -r);
            if (r == 0)
            {
                zeros++;
            }
        }
        double m = HllM;
        double estimate = 0.7213 / (1.0 + 1.079 / m) * m * m / z;
        if (estimate <= 2.5 * m && zeros > 0)
        {
            estimate = m * Math.Log(m / zeros);
        }
        long rounded = (long)Math.Floor(estimate + 0.5);
        uint hash = Checksum.H31(registers.Select(r => (long)r));
        return Format($"{n}:{rounded}:{hash}");
    }

    private const int SkipMaxLevel = 24;
    private const uint SkipLevelSeed = 88675123;

    private static SkipResult SkipList(long[] keys, long[] lookups)
    {
        var rng = new XorShift32(SkipLevelSeed);
        var head = new SkipNode(-1, SkipMaxLevel);
        int level = 1;
        var update = new SkipNode[SkipMaxLevel];
        Array.Fill(update, head);
        long heights = 0;
        foreach (long key in keys)
        {
            SkipNode x = head;
            for (int i = level - 1; i >= 0; i--)
            {
                SkipNode? next = x.Next[i];
                while (next is not null && next.Key < key)
                {
                    x = next;
                    next = x.Next[i];
                }
                update[i] = x;
            }
            SkipNode? found = x.Next[0];
            if (found is not null && found.Key == key)
            {
                continue;
            }
            int height = 1;
            while (height < SkipMaxLevel && (rng.Next() & 1) != 0)
            {
                height++;
            }
            for (int i = level; i < height; i++)
            {
                update[i] = head;
            }
            level = Math.Max(level, height);
            var node = new SkipNode(key, height);
            for (int i = 0; i < height; i++)
            {
                node.Next[i] = update[i].Next[i];
                update[i].Next[i] = node;
            }
            heights += height;
        }
        int hits = 0;
        foreach (long key in lookups)
        {
            SkipNode x = head;
            for (int i = level - 1; i >= 0; i--)
            {
                SkipNode? next = x.Next[i];
                while (next is not null && next.Key < key)
                {
                    x = next;
                    next = x.Next[i];
                }
            }
            SkipNode? found = x.Next[0];
            if (found is not null && found.Key == key)
            {
                hits++;
            }
        }
        return new SkipResult(head, keys.Length, hits, heights);
    }

    private static string SkipChecksum(SkipResult result)
    {
        var ordered = new List<long>(result.Count);
        for (SkipNode? x = result.Head.Next[0]; x is not null; x = x.Next[0])
        {
            ordered.Add(x.Key);
        }
        return Format($"{result.Count}:{result.Hits}:{result.Heights}:{Checksum.H31(ordered)}");
    }

    private const long CycleMod = 67108859;
    private const long CycleC = 12345;

    private static long[] CycleStarts(int n)
    {
        var rng = new XorShift32();
        var starts = new long[n];
        for (int i = 0; i < n; i++)
        {
            starts[i] = rng.Next() % CycleMod;
        }
        return starts;
    }

    private static long Step(long x) => (x * x + CycleC) % CycleMod;

    private static string Cycle(long[] starts)
    {
        long totalMu = 0;
        long totalLam = 0;
        long longest = 0;
        foreach (long x0 in starts)
        {
            long tortoise = Step(x0);
            long hare = Step(tortoise);
            while (tortoise != hare)
            {
                tortoise = Step(tortoise);
                hare = Step(Step(hare));
            }
            long mu = 0;
            tortoise = x0;
            while (tortoise != hare)
            {
                tortoise = Step(tortoise);
                hare = Step(hare);
                mu++;
            }
            long lam = 1;
            hare = Step(tortoise);
            while (tortoise != hare)
            {
                hare = Step(hare);
                lam++;
            }
            totalMu += mu;
            totalLam += lam;
            longest = Math.Max(longest, mu + lam);
        }
        return Format($"{starts.Length}:{totalMu}:{totalLam}:{longest}");
    }

    private const int LifeSide = 512;

    private static byte[] LifeInput()
    {
        var rng = new XorShift32();
        var grid = new byte[LifeSide * LifeSide];
        for (int i = 0; i < grid.Length; i++)
        {
            grid[i] = (byte)(rng.Next() % 4 == 0 ? 1 : 0);
        }
        return grid;
    }

    private static byte[] Life(byte[] grid, int generations)
    {
        const int w = LifeSide;
        byte[] cur = (byte[])grid.Clone();
        var next = new byte[grid.Length];
        for (int g = 0; g < generations; g++)
        {
            for (int y = 0; y < w; y++)
            {
                int up = (y + w - 1) % w * w;
                int row = y * w;
                int down = (y + 1) % w * w;
                for (int x = 0; x < w; x++)
                {
                    int left = (x + w - 1) % w;
                    int right = (x + 1) % w;
                    int n = cur[up + left] + cur[up + x] + cur[up + right]
                        + cur[row + left] + cur[row + right]
                        + cur[down + left] + cur[down + x] + cur[down + right];
                    next[row + x] = (byte)(n == 3 || (n == 2 && cur[row + x] != 0) ? 1 : 0);
                }
            }
            (cur, next) = (next, cur);
        }
        return cur;
    }

    private static string LifeChecksum(int generations, byte[] grid) =>
        Format($"{generations}:{grid.Sum(b => (long)b)}:{Checksum.Fnv1a(grid)}");

    private static readonly int[] Windows = [10, 100, 1000, 10000, 100000];

    private static string WindowMax(long[] values)
    {
        int n = values.Length;
        var dq = new int[n];
        var hashes = new List<string>(Windows.Length);
        foreach (int w in Windows)
        {
            int head = 0;
            int tail = 0;
            uint h = 0;
            for (int i = 0; i < n; i++)
            {
                long v = values[i];
                while (tail > head && values[dq[tail - 1]] <= v)
                {
                    tail--;
                }
                dq[tail++] = i;
                if (dq[head] <= i - w)
                {
                    head++;
                }
                if (i >= w - 1)
                {
                    h = h * 31 + (uint)values[dq[head]];
                }
            }
            hashes.Add(h.ToString(CultureInfo.InvariantCulture));
        }
        return Format($"{n}:{string.Join(':', hashes)}");
    }

    private static int[] Terrain(int side)
    {
        var rng = new XorShift32();
        var heights = new int[side * side];
        for (int i = 0; i < heights.Length; i++)
        {
            heights[i] = (int)(rng.Next() % 1000);
        }
        return heights;
    }

    private static string RainWater(int[] heights)
    {
        int n = heights.Length;
        int side = (int)Math.Sqrt(n);
        var seen = new bool[n];
        var heap = new PriorityQueue<long, long>();
        for (int idx = 0; idx < n; idx++)
        {
            int r = idx / side;
            int c = idx % side;
            if (r == 0 || r == side - 1 || c == 0 || c == side - 1)
            {
                seen[idx] = true;
                long key = (long)heights[idx] * n + idx;
                heap.Enqueue(key, key);
            }
        }
        long water = 0;
        long wet = 0;
        (int Dr, int Dc)[] moves = [(-1, 0), (1, 0), (0, -1), (0, 1)];
        while (heap.Count > 0)
        {
            long top = heap.Dequeue();
            long level = top / n;
            int idx = (int)(top % n);
            int r = idx / side;
            int c = idx % side;
            foreach ((int dr, int dc) in moves)
            {
                int nr = r + dr;
                int nc = c + dc;
                if (nr < 0 || nr >= side || nc < 0 || nc >= side)
                {
                    continue;
                }
                int neighbour = nr * side + nc;
                if (seen[neighbour])
                {
                    continue;
                }
                seen[neighbour] = true;
                long h = heights[neighbour];
                if (h < level)
                {
                    water += level - h;
                    wet++;
                    h = level;
                }
                long key = h * n + neighbour;
                heap.Enqueue(key, key);
            }
        }
        return Format($"{side}:{water}:{wet}");
    }

    private static string Collatz(int n)
    {
        var cache = new int[n];
        cache[1] = 1;
        long best = 1;
        long bestLength = 1;
        long total = 1;
        for (int start = 2; start < n; start++)
        {
            long x = start;
            int steps = 0;
            while (x >= start)
            {
                x = x % 2 == 0 ? x / 2 : 3 * x + 1;
                steps++;
            }
            int length = steps + cache[x];
            cache[start] = length;
            total += length;
            if (length > bestLength)
            {
                best = start;
                bestLength = length;
            }
        }
        return Format($"{n}:{best}:{bestLength}:{total}");
    }
}
