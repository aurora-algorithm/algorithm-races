// Race: lru (csharp)
// Extracted from suite/csharp/AlgoTrees.cs — build and run via suite/run.py

namespace Kernels;

public static class AlgoTrees
{

    public static (int Capacity, int[] Keys) LruKeys(int n)
    {
        int capacity = Math.Max(1, n / 100);
        var rng = new XorShift32();
        var keys = new int[n];
        for (int i = 0; i < n; i++)
        {
            keys[i] = (int)(rng.Next() % (uint)(2 * capacity));
        }
        return (capacity, keys);
    }

    /// <summary>.NET has no linked hash map: a Dictionary finds the key's node in a LinkedList, newest first.</summary>
    public static string Lru(int capacity, int[] keys)
    {
        var order = new LinkedList<int>();
        var nodes = new Dictionary<int, LinkedListNode<int>>(capacity);
        int hits = 0;
        foreach (int k in keys)
        {
            if (nodes.TryGetValue(k, out LinkedListNode<int>? node))
            {
                order.Remove(node);
                order.AddFirst(node);
                hits++;
                continue;
            }
            if (order.Count == capacity)
            {
                LinkedListNode<int> oldest = order.Last!;
                nodes.Remove(oldest.Value);
                order.RemoveLast();
            }
            nodes[k] = order.AddFirst(k);
        }
        var oldestFirst = new List<long>(order.Count);
        for (LinkedListNode<int>? n = order.Last; n is not null; n = n.Previous)
        {
            oldestFirst.Add(n.Value);
        }
        return Algo.Invariant($"{hits}:{keys.Length - hits}:{Checksum.H31(oldestFirst)}");
    }

}
