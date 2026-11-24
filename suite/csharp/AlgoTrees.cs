namespace Kernels;

/// <summary>Tree and cache kernels of the algo group: BST, SortedDictionary, trie, LRU cache.</summary>
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

    private static List<long> InOrder(Node? root, int size)
    {
        var keys = new List<long>(size);
        var stack = new Stack<Node>();
        Node? node = root;
        while (node is not null || stack.Count > 0)
        {
            while (node is not null)
            {
                stack.Push(node);
                node = node.Left;
            }
            node = stack.Pop();
            keys.Add(node.Key);
            node = node.Right;
        }
        return keys;
    }

    public static string OrderedMap(long[] inserts, long[] lookups)
    {
        var counts = new SortedDictionary<long, long>();
        foreach (long key in inserts)
        {
            counts[key] = counts.GetValueOrDefault(key) + 1;
        }
        int hits = 0;
        foreach (long key in lookups)
        {
            if (counts.ContainsKey(key))
            {
                hits++;
            }
        }
        return Algo.Invariant($"{counts.Count}:{hits}:{Checksum.H31(counts.Keys)}");
    }

    public static (string[] Inserted, string[] Prefixes) Words(int n)
    {
        var rng = new XorShift32();
        var inserted = new string[n];
        for (int i = 0; i < n; i++)
        {
            inserted[i] = Word(rng, 3, 8);
        }
        var prefixes = new string[n];
        for (int i = 0; i < n; i++)
        {
            prefixes[i] = Word(rng, 1, 4);
        }
        return (inserted, prefixes);
    }

    private static string Word(XorShift32 rng, uint minLength, uint spread)
    {
        var chars = new char[minLength + rng.Next() % spread];
        for (int c = 0; c < chars.Length; c++)
        {
            chars[c] = (char)('a' + rng.Next() % 26);
        }
        return new string(chars);
    }

    private sealed class TrieNode
    {
        public long Count { get; set; }
        public TrieNode?[] Children { get; } = new TrieNode?[26];
    }

    public static string Trie(string[] inserted, string[] prefixes)
    {
        var root = new TrieNode();
        long nodes = 0;
        foreach (string word in inserted)
        {
            TrieNode node = root;
            foreach (char ch in word)
            {
                TrieNode? child = node.Children[ch - 'a'];
                if (child is null)
                {
                    child = new TrieNode();
                    node.Children[ch - 'a'] = child;
                    nodes++;
                }
                child.Count++;
                node = child;
            }
        }
        long total = 0;
        foreach (string prefix in prefixes)
        {
            TrieNode? node = root;
            foreach (char ch in prefix)
            {
                node = node.Children[ch - 'a'];
                if (node is null)
                {
                    break;
                }
            }
            total += node?.Count ?? 0;
        }
        return Algo.Invariant($"{nodes}:{total}");
    }

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
