// Race: kruskal (go)
// Extracted from suite/go/algo.go — build and run via suite/run.py

package main
import (
	"cmp"
	"fmt"
	"slices"
)

type edge struct {
	w    uint32
	u, v int
}

func edgeList(n int) []edge {
	rng := newRNG()
	edges := make([]edge, 4*n)
	for i := range edges {
		u := int(rng.Next() % uint32(n))
		v := int(rng.Next() % uint32(n))
		edges[i] = edge{w: rng.Next() % 1000000, u: u, v: v}
	}
	return edges
}

func find(parent []int, x int) int {
	root := x
	for parent[root] != root {
		root = parent[root]
	}
	for parent[x] != root {
		parent[x], x = root, parent[x]
	}
	return root
}

func kruskal(n int, edges []edge) string {
	slices.SortFunc(edges, func(a, b edge) int { return cmp.Compare(a.w, b.w) })
	parent := make([]int, n)
	for i := range parent {
		parent[i] = i
	}
	rank := make([]uint8, n)
	count, total := 0, uint64(0)
	for _, e := range edges {
		ru, rv := find(parent, e.u), find(parent, e.v)
		if ru == rv {
			continue
		}
		if rank[ru] < rank[rv] {
			ru, rv = rv, ru
		}
		parent[rv] = ru
		if rank[ru] == rank[rv] {
			rank[ru]++
		}
		count++
		total += uint64(e.w)
	}
	return fmt.Sprintf("%d:%d", count, total)
}

func kruskalKernel(c config) (string, []float64, error) {
	base := edgeList(c.size)
	run := func(e []edge) string { return kruskal(c.size, e) }
	out, times := measure(c.repeats, func() []edge { return slices.Clone(base) }, run)
	return out, times, nil
}
