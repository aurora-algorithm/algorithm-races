using System.Globalization;
using System.Runtime.CompilerServices;
using System.Text;

namespace Kernels;

/// <summary>
/// The -arena kernels (kernels/SPEC.md section 12, "Arena rows"): bst, trie and binary-trees with every node in one
/// growable struct array and 32-bit index links, the same algorithms and checksums as the class versions.
/// </summary>
public static class Arena
{
    private const int FirstCapacity = 16;

    /// <summary>The "no child" link of the BST and the binary trees.</summary>
    private const int Nil = -1;

    private static readonly Dictionary<string, Func<Options, Measurement>> Kernels = new()
    {
        ["bst-arena"] = o => KernelTable.Measure(o, () => Inputs.HashMap(o.Size), m => Bst(m.Inserts, m.Lookups), s => s),
        ["trie-arena"] = o =>
            KernelTable.Measure(o, () => AlgoTrees.Words(o.Size), w => Trie(w.Inserted, w.Prefixes), s => s),
        ["binary-trees-arena"] = o => KernelTable.Measure(
            o, () => o.Size, BinaryTrees, s => Checksum.Fnv1a(s).ToString(CultureInfo.InvariantCulture)),
    };

    public static Func<Options, Measurement>? Find(string name) => Kernels.GetValueOrDefault(name);

    /// <summary>Appends one node, doubling the array when it is full; returns the node's index.</summary>
    private static int Push<T>(ref T[] nodes, ref int count, T node)
    {
        if (count == nodes.Length)
        {
            Array.Resize(ref nodes, nodes.Length * 2);
        }
        nodes[count] = node;
        return count++;
    }

    private struct BstNode(long key)
    {
        public readonly long Key = key;
        public int Count = 1;
        public int Left = Nil;
        public int Right = Nil;
    }

    public static string Bst(long[] inserts, long[] lookups)
    {
        var nodes = new BstNode[FirstCapacity];
        int size = 0;
        foreach (long k in inserts)
        {
            if (size == 0)
            {
                Push(ref nodes, ref size, new BstNode(k));
                continue;
            }
            int cur = 0;
            while (true)
            {
                ref BstNode node = ref nodes[cur];
                if (k < node.Key)
                {
                    if (node.Left == Nil)
                    {
                        node.Left = size;
                        Push(ref nodes, ref size, new BstNode(k));
                        break;
                    }
                    cur = node.Left;
                }
                else if (k > node.Key)
                {
                    if (node.Right == Nil)
                    {
                        node.Right = size;
                        Push(ref nodes, ref size, new BstNode(k));
                        break;
                    }
                    cur = node.Right;
                }
                else
                {
                    node.Count++;
                    break;
                }
            }
        }
        int hits = 0;
        foreach (long k in lookups)
        {
            int cur = size == 0 ? Nil : 0;
            while (cur != Nil && nodes[cur].Key != k)
            {
                cur = k < nodes[cur].Key ? nodes[cur].Left : nodes[cur].Right;
            }
            if (cur != Nil)
            {
                hits++;
            }
        }
        return Algo.Invariant($"{size}:{hits}:{Checksum.H31(InOrder(nodes, size))}");
    }

    private static List<long> InOrder(BstNode[] nodes, int size)
    {
        var keys = new List<long>(size);
        var stack = new Stack<int>();
        int cur = size == 0 ? Nil : 0;
        while (cur != Nil || stack.Count > 0)
        {
            while (cur != Nil)
            {
                stack.Push(cur);
                cur = nodes[cur].Left;
            }
            cur = stack.Pop();
            keys.Add(nodes[cur].Key);
            cur = nodes[cur].Right;
        }
        return keys;
    }

    [InlineArray(26)]
    private struct Children
    {
        private int _first;
    }

    /// <summary>Node 0 is the root and never anyone's child, so a child link of 0 means no child.</summary>
    private struct TrieNode
    {
        public int Count;
        public Children Children;
    }

    public static string Trie(string[] inserted, string[] prefixes)
    {
        var nodes = new TrieNode[FirstCapacity];
        int count = 1;
        foreach (string word in inserted)
        {
            int node = 0;
            foreach (char ch in word)
            {
                int i = ch - 'a';
                if (nodes[node].Children[i] == 0)
                {
                    nodes[node].Children[i] = count;
                    Push(ref nodes, ref count, default);
                }
                node = nodes[node].Children[i];
                nodes[node].Count++;
            }
        }
        long total = 0;
        foreach (string prefix in prefixes)
        {
            // A prefix has at least one letter, so node 0 after the walk means it is absent.
            int node = 0;
            foreach (char ch in prefix)
            {
                node = nodes[node].Children[ch - 'a'];
                if (node == 0)
                {
                    break;
                }
            }
            if (node != 0)
            {
                total += nodes[node].Count;
            }
        }
        return Algo.Invariant($"{count - 1}:{total}");
    }

    /// <summary>A binary tree's node; a leaf's links are Nil.</summary>
    private readonly record struct TreeNode(int Left, int Right);

    /// <summary>One tree's nodes, children pushed before their parent.</summary>
    private sealed class TreeArena
    {
        private TreeNode[] _nodes = new TreeNode[FirstCapacity];
        private int _count;

        public int Make(int depth)
        {
            int left = Nil;
            int right = Nil;
            if (depth > 0)
            {
                left = Make(depth - 1);
                right = Make(depth - 1);
            }
            return Push(ref _nodes, ref _count, new TreeNode(left, right));
        }

        public int Check(int index)
        {
            TreeNode node = _nodes[index];
            return node.Left == Nil ? 1 : 1 + Check(node.Left) + Check(node.Right);
        }
    }

    private static int TreeCheck(int depth)
    {
        var tree = new TreeArena();
        return tree.Check(tree.Make(depth));
    }

    public static string BinaryTrees(int n)
    {
        int maxDepth = Math.Max(BenchmarksGame.MinDepth + 2, n);
        int stretch = maxDepth + 1;
        var output = new StringBuilder();
        var culture = CultureInfo.InvariantCulture;
        output.Append(culture, $"stretch tree of depth {stretch}\t check: {TreeCheck(stretch)}\n");
        var longLived = new TreeArena();
        int longRoot = longLived.Make(maxDepth);
        for (int depth = BenchmarksGame.MinDepth; depth <= maxDepth; depth += 2)
        {
            int iterations = 1 << (maxDepth - depth + BenchmarksGame.MinDepth);
            int check = 0;
            for (int i = 0; i < iterations; i++)
            {
                check += TreeCheck(depth);
            }
            output.Append(culture, $"{iterations}\t trees of depth {depth}\t check: {check}\n");
        }
        output.Append(culture, $"long lived tree of depth {maxDepth}\t check: {longLived.Check(longRoot)}\n");
        return output.ToString();
    }
}
