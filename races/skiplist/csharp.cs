// Race: skiplist (csharp)
// Extracted from suite/csharp/Puzzle.cs — build and run via suite/run.py

using System.Globalization;
using System.Numerics;
using static Kernels.PuzzleText;
namespace Kernels;

public sealed record SkipResult(SkipNode Head, int Count, int Hits, long Heights);

public sealed class SkipNode(long key, int height)
{
    public long Key { get; } = key;

    public SkipNode?[] Next { get; } = new SkipNode?[height];
}

public static class Puzzle
{

    internal static string Format(FormattableString text) => text.ToString(CultureInfo.InvariantCulture);

    private const int SkipMaxLevel = 24;

    private const uint SkipLevelSeed = 88675123;

    private static SkipResult SkipList(long[] keys, long[] lookups)
    {
        var rng = new XorShift32(SkipLevelSeed);
        var head = new SkipNode(-1, SkipMaxLevel);
        int level = 1;
        var update = new SkipNode[SkipMaxLevel];
        Array.Fill(update, head);
        long heights = 0;
        foreach (long key in keys)
        {
            SkipNode x = head;
            for (int i = level - 1; i >= 0; i--)
            {
                SkipNode? next = x.Next[i];
                while (next is not null && next.Key < key)
                {
                    x = next;
                    next = x.Next[i];
                }
                update[i] = x;
            }
            SkipNode? found = x.Next[0];
            if (found is not null && found.Key == key)
            {
                continue;
            }
            int height = 1;
            while (height < SkipMaxLevel && (rng.Next() & 1) != 0)
            {
                height++;
            }
            for (int i = level; i < height; i++)
            {
                update[i] = head;
            }
            level = Math.Max(level, height);
            var node = new SkipNode(key, height);
            for (int i = 0; i < height; i++)
            {
                node.Next[i] = update[i].Next[i];
                update[i].Next[i] = node;
            }
            heights += height;
        }
        int hits = 0;
        foreach (long key in lookups)
        {
            SkipNode x = head;
            for (int i = level - 1; i >= 0; i--)
            {
                SkipNode? next = x.Next[i];
                while (next is not null && next.Key < key)
                {
                    x = next;
                    next = x.Next[i];
                }
            }
            SkipNode? found = x.Next[0];
            if (found is not null && found.Key == key)
            {
                hits++;
            }
        }
        return new SkipResult(head, keys.Length, hits, heights);
    }

    private static string SkipChecksum(SkipResult result)
    {
        var ordered = new List<long>(result.Count);
        for (SkipNode? x = result.Head.Next[0]; x is not null; x = x.Next[0])
        {
            ordered.Add(x.Key);
        }
        return Format($"{result.Count}:{result.Hits}:{result.Heights}:{Checksum.H31(ordered)}");
    }

}
