using System.Globalization;

namespace Kernels;

/// <summary>Classic algorithm kernels (kernels/SPEC.md section 12, group algo).</summary>
public static class Algo
{
    private const int KnapsackItems = 1000;
    private const int PatternLength = 8;
    private const string Acgt = "acgt";

    // The 64 most common English words (kernels/SPEC.md section 12, kmp-en).
    private static readonly string[] Words =
        ("the be to of and a in that have i it for not on with he as you do at this but his by from they we say her she "
        + "or an will my one all would there their what so up out if about who get which go me when make can like time no "
        + "just him know take people into year your").Split(' ');

    private static readonly Dictionary<string, Func<Options, Measurement>> Kernels = new()
    {
        ["bfs-grid"] = o => KernelTable.Measure(o, () => AlgoGraphs.Grid(o.Size), w => AlgoGraphs.BfsGrid(o.Size, w), Same),
        ["dijkstra"] = AlgoGraphs.DijkstraKernel,
        ["kruskal"] = o => KernelTable.MeasureWithExtra(
            o, () => AlgoGraphs.EdgeList(o.Size), e => (AlgoGraphs.Edge[])e.Clone(), e => AlgoGraphs.Kruskal(o.Size, e), Plain),
        ["bst"] = o => KernelTable.Measure(o, () => Inputs.HashMap(o.Size), m => AlgoTrees.Bst(m.Inserts, m.Lookups), Same),
        ["ordered-map"] = o =>
            KernelTable.Measure(o, () => Inputs.HashMap(o.Size), m => AlgoTrees.OrderedMap(m.Inserts, m.Lookups), Same),
        ["trie"] = o => KernelTable.Measure(o, () => AlgoTrees.Words(o.Size), w => AlgoTrees.Trie(w.Inserted, w.Prefixes), Same),
        ["lru"] = o => KernelTable.Measure(o, () => AlgoTrees.LruKeys(o.Size), k => AlgoTrees.Lru(k.Capacity, k.Keys), Same),
        ["edit-distance"] = o => KernelTable.Measure(o, () => Dna(o.Size, 2), ab => EditDistance(ab[0], ab[1]), Same),
        ["knapsack"] = o => KernelTable.Measure(o, Items, t => Knapsack(o.Size, t), Same),
        ["lis"] = o => KernelTable.Measure(o, () => Inputs.Ints(o.Size), Lis, Same),
        ["kmp"] = o => KernelTable.Measure(o, () => Text(o.Size), t => Kmp(t.Text, t.Pattern), Same),
        ["str-find"] = o => KernelTable.Measure(o, () => Text(o.Size), t => StrFind(t.Text, t.Pattern), Same),
        ["kmp-en"] = o => KernelTable.Measure(o, () => TextEn(o.Size), t => Kmp(t.Text, t.Pattern), Same),
        ["str-find-en"] = o => KernelTable.Measure(o, () => TextEn(o.Size), t => StrFind(t.Text, t.Pattern), Same),
        ["fib"] = o => KernelTable.Measure(o, () => o.Size, n => Invariant($"{Fib(n)}"), Same),
        ["nqueens"] = o => KernelTable.Measure(o, () => o.Size, n => Invariant($"{NQueens(n)}"), Same),
        ["binary-search"] = o => KernelTable.Measure(o, () => SortedArray(o.Size), s => BinarySearch(s.A, s.Queries), Same),
    };

    public static Func<Options, Measurement>? Find(string name) => Kernels.GetValueOrDefault(name);

    internal static string Invariant(FormattableString text) => text.ToString(CultureInfo.InvariantCulture);

    private static string Same(string s) => s;

    private static Measurement Plain(string s) => new(s, []);

    private static string[] Dna(int length, int count)
    {
        var rng = new XorShift32();
        var strings = new string[count];
        for (int s = 0; s < count; s++)
        {
            var chars = new char[length];
            for (int i = 0; i < length; i++)
            {
                chars[i] = Acgt[(int)(rng.Next() % 4)];
            }
            strings[s] = new string(chars);
        }
        return strings;
    }

    public static string EditDistance(string a, string b)
    {
        var prev = new int[b.Length + 1];
        var cur = new int[b.Length + 1];
        for (int j = 0; j <= b.Length; j++)
        {
            prev[j] = j;
        }
        for (int i = 0; i < a.Length; i++)
        {
            cur[0] = i + 1;
            for (int j = 0; j < b.Length; j++)
            {
                int cost = a[i] == b[j] ? 0 : 1;
                cur[j + 1] = Math.Min(Math.Min(prev[j + 1] + 1, cur[j] + 1), prev[j] + cost);
            }
            (prev, cur) = (cur, prev);
        }
        return Invariant($"{prev[b.Length]}");
    }

    private static (int Weight, long Value)[] Items()
    {
        var rng = new XorShift32();
        var items = new (int Weight, long Value)[KnapsackItems];
        for (int i = 0; i < items.Length; i++)
        {
            int weight = 1 + (int)(rng.Next() % 1000);
            items[i] = (weight, 1 + rng.Next() % 1000);
        }
        return items;
    }

    public static string Knapsack(int capacity, (int Weight, long Value)[] items)
    {
        var best = new long[capacity + 1];
        foreach ((int weight, long value) in items)
        {
            for (int c = capacity; c >= weight; c--)
            {
                if (best[c - weight] + value > best[c])
                {
                    best[c] = best[c - weight] + value;
                }
            }
        }
        return Invariant($"{best[capacity]}:{Checksum.H31(best)}");
    }

    public static string Lis(long[] values)
    {
        var tails = new long[16];
        int length = 0;
        foreach (long x in values)
        {
            int i = Array.BinarySearch(tails, 0, length, x);
            if (i < 0)
            {
                i = ~i;
            }
            if (i == tails.Length)
            {
                Array.Resize(ref tails, tails.Length * 2);
            }
            tails[i] = x;
            if (i == length)
            {
                length++;
            }
        }
        return Invariant($"{length}:{Checksum.H31(tails.Take(length))}");
    }

    private static (string Text, string Pattern) Text(int n)
    {
        string text = Dna(n, 1)[0];
        return (text, text.Substring(n / 2, PatternLength));
    }

    private static string English(int n)
    {
        var rng = new XorShift32();
        var text = new System.Text.StringBuilder(n + 16);
        while (text.Length < n)
        {
            uint a = rng.Next() % 64;
            uint b = rng.Next() % 64;
            text.Append(Words[Math.Min(a, b)]);
            text.Append((rng.Next() % 16) switch
            {
                0 => ". ",
                1 => ", ",
                _ => " ",
            });
        }
        return text.ToString(0, n);
    }

    private static (string Text, string Pattern) TextEn(int n)
    {
        string text = English(n);
        return (text, text.Substring(n / 2, PatternLength));
    }

    private static string Matches(int count, int first, int last) => Invariant($"{count}:{first}:{last}");

    public static string Kmp(string text, string pattern)
    {
        int m = pattern.Length;
        var fail = new int[m];
        int k = 0;
        for (int i = 1; i < m; i++)
        {
            while (k > 0 && pattern[i] != pattern[k])
            {
                k = fail[k - 1];
            }
            if (pattern[i] == pattern[k])
            {
                k++;
            }
            fail[i] = k;
        }
        int count = 0, first = -1, last = -1;
        k = 0;
        for (int i = 0; i < text.Length; i++)
        {
            char ch = text[i];
            while (k > 0 && ch != pattern[k])
            {
                k = fail[k - 1];
            }
            if (ch == pattern[k])
            {
                k++;
            }
            if (k == m)
            {
                last = i + 1 - m;
                if (first < 0)
                {
                    first = last;
                }
                count++;
                k = fail[k - 1];
            }
        }
        return Matches(count, first, last);
    }

    public static string StrFind(string text, string pattern)
    {
        int count = 0, first = -1, last = -1;
        int pos = text.IndexOf(pattern, StringComparison.Ordinal);
        while (pos >= 0)
        {
            if (first < 0)
            {
                first = pos;
            }
            last = pos;
            count++;
            pos = text.IndexOf(pattern, pos + 1, StringComparison.Ordinal);
        }
        return Matches(count, first, last);
    }

    public static long Fib(int n) => n < 2 ? n : Fib(n - 1) + Fib(n - 2);

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

    private static (long[] A, long[] Queries) SortedArray(int n)
    {
        var rng = new XorShift32();
        var a = new long[n];
        long v = 0;
        for (int i = 0; i < n; i++)
        {
            v += 1 + rng.Next() % 4;
            a[i] = v;
        }
        var queries = new long[n];
        for (int i = 0; i < n; i++)
        {
            queries[i] = rng.Next() % (v + 1);
        }
        return (a, queries);
    }

    public static string BinarySearch(long[] a, long[] queries)
    {
        int n = a.Length;
        long hits = 0, total = 0;
        foreach (long q in queries)
        {
            int lo = 0, hi = n;
            while (lo < hi)
            {
                int mid = (lo + hi) / 2;
                if (a[mid] < q)
                {
                    lo = mid + 1;
                }
                else
                {
                    hi = mid;
                }
            }
            if (lo < n && a[lo] == q)
            {
                hits++;
            }
            total += lo;
        }
        return Invariant($"{hits}:{total}");
    }
}
