// Race: bst (csharp)
// Extracted from suite/csharp/AlgoTrees.cs — build and run via suite/run.py

namespace Kernels;

public static class AlgoTrees
{

    private sealed class Node(long key)
    {
        public long Key { get; } = key;
        public int Count { get; set; } = 1;
        public Node? Left { get; set; }
        public Node? Right { get; set; }
    }

    public static string Bst(long[] inserts, long[] lookups)
    {
        Node? root = null;
        int size = 0;
        foreach (long k in inserts)
        {
            if (root is null)
            {
                root = new Node(k);
                size++;
                continue;
            }
            Node node = root;
            while (true)
            {
                if (k < node.Key)
                {
                    if (node.Left is null)
                    {
                        node.Left = new Node(k);
                        size++;
                        break;
                    }
                    node = node.Left;
                }
                else if (k > node.Key)
                {
                    if (node.Right is null)
                    {
                        node.Right = new Node(k);
                        size++;
                        break;
                    }
                    node = node.Right;
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
            Node? node = root;
            while (node is not null && node.Key != k)
            {
                node = k < node.Key ? node.Left : node.Right;
            }
            if (node is not null)
            {
                hits++;
            }
        }
        return Algo.Invariant($"{size}:{hits}:{Checksum.H31(InOrder(root, size))}");
    }

}
