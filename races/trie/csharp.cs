// Race: trie (csharp)
// Extracted from suite/csharp/AlgoTrees.cs — build and run via suite/run.py

namespace Kernels;

public static class AlgoTrees
{

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

}
