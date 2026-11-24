// Race: bst-arena (csharp)
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

    /// <summary>A binary tree's node; a leaf's links are Nil.</summary>
    private readonly record struct TreeNode(int Left, int Right);

}
