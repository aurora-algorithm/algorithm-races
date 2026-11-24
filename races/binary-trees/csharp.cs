// Race: binary-trees (csharp)
// Extracted from suite/csharp/BenchmarksGame.cs — build and run via suite/run.py

using System.Globalization;
using System.Text;
namespace Kernels;

public sealed class Node(Node? left, Node? right)
{
    public static Node Make(int depth) =>
        depth == 0 ? new Node(null, null) : new Node(Make(depth - 1), Make(depth - 1));

    public int Check() => left is null || right is null ? 1 : 1 + left.Check() + right.Check();
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

}
