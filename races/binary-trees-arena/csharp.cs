// Race: binary-trees-arena (csharp)
// Extracted from suite/csharp/Arena.cs — build and run via suite/run.py

using System.Globalization;
using System.Runtime.CompilerServices;
using System.Text;
namespace Kernels;

public static class Arena
{

    private const int FirstCapacity = 16;

    /// <summary>The "no child" link of the BST and the binary trees.</summary>
    private const int Nil = -1;

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
