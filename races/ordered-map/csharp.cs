// Race: ordered-map (csharp)
// Extracted from suite/csharp/AlgoTrees.cs — build and run via suite/run.py

namespace Kernels;

public static class AlgoTrees
{

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

}
