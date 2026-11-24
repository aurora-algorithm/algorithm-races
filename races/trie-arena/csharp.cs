// Race: trie-arena (csharp)
// Extracted from suite/csharp/Arena.cs — build and run via suite/run.py

using System.Globalization;
using System.Runtime.CompilerServices;
using System.Text;
namespace Kernels;

public static class Arena
{

    private const int FirstCapacity = 16;

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

}
