package main

// The ssearch kernels (../SPEC.md section 16): lower bound in small sorted arrays, a plain loop
// against binary search, at every size from 4 to 1024. Kernel names are ssearch.go.WAY.

import (
	"fmt"
	"math"
	"slices"
	"sort"
	"time"
)

var ssearchSizes = []int{4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024}

// A queryLoop runs every query through one way. Each way has its own loop so the search is a
// direct call the compiler can inline.
type queryLoop func(a, queries []int64) (hits, total uint64)

func linearSearch(a []int64, q int64) int {
	for i, x := range a {
		if x >= q {
			return i
		}
	}
	return len(a)
}

func linearCount(a []int64, q int64) int {
	c := 0
	for _, x := range a {
		if x < q {
			c++
		}
	}
	return c
}

func binaryByHand(a []int64, q int64) int {
	lo, hi := 0, len(a)
	for lo < hi {
		mid := (lo + hi) / 2
		if a[mid] < q {
			lo = mid + 1
		} else {
			hi = mid
		}
	}
	return lo
}

// binaryBranchless is the branch-free lower bound of SPEC section 16 ("branchless"): each step moves
// base by half or by nothing, picked by the sign bit of a[base+half]-q, so no jump depends on the
// data (Go 1.27 keeps a conditional jump for the plain if form). The bench's values are small, so the
// subtraction cannot overflow.
func binaryBranchless(a []int64, q int64) int {
	base, n := 0, len(a)
	for n > 1 {
		half := n / 2
		base += half & int((a[base+half]-q)>>63)
		n -= half
	}
	if n == 1 {
		base += 1 & int((a[base]-q)>>63)
	}
	return base
}

func tally(a []int64, q int64, lo int, hits, total *uint64) {
	if lo < len(a) && a[lo] == q {
		*hits++
	}
	*total += uint64(lo)
}

func queryLinear(a, queries []int64) (h, t uint64) {
	for _, q := range queries {
		tally(a, q, linearSearch(a, q), &h, &t)
	}
	return h, t
}

func queryLinearCount(a, queries []int64) (h, t uint64) {
	for _, q := range queries {
		tally(a, q, linearCount(a, q), &h, &t)
	}
	return h, t
}

func queryBinary(a, queries []int64) (h, t uint64) {
	for _, q := range queries {
		tally(a, q, binaryByHand(a, q), &h, &t)
	}
	return h, t
}

func queryBranchless(a, queries []int64) (h, t uint64) {
	for _, q := range queries {
		tally(a, q, binaryBranchless(a, q), &h, &t)
	}
	return h, t
}

func querySortSearch(a, queries []int64) (h, t uint64) {
	for _, q := range queries {
		tally(a, q, sort.Search(len(a), func(i int) bool { return a[i] >= q }), &h, &t)
	}
	return h, t
}

func querySlicesBinarySearch(a, queries []int64) (h, t uint64) {
	for _, q := range queries {
		i, _ := slices.BinarySearch(a, q)
		tally(a, q, i, &h, &t)
	}
	return h, t
}

// An ssearchWay is one kernel: its query loop, and whether every query repeats queries[2] (the
// ".same" controls: each jump goes the same way every time, so branch prediction always succeeds).
type ssearchWay struct {
	loop queryLoop
	same bool
}

var ssearchKernels = map[string]ssearchWay{
	"ssearch.go.linear":                   {queryLinear, false},
	"ssearch.go.linear-count":             {queryLinearCount, false},
	"ssearch.go.binary":                   {queryBinary, false},
	"ssearch.go.sort.Search":              {querySortSearch, false},
	"ssearch.go.slices.BinarySearch":      {querySlicesBinarySearch, false},
	"ssearch.go.linear.same":              {queryLinear, true},
	"ssearch.go.binary.same":              {queryBinary, true},
	"ssearch.go.sort.Search.same":         {querySortSearch, true},
	"ssearch.go.slices.BinarySearch.same": {querySlicesBinarySearch, true},
	"ssearch.go.branchless":               {queryBranchless, false},
	"ssearch.go.branchless.same":          {queryBranchless, true},
}

func medianOf(v []float64) float64 {
	s := slices.Clone(v)
	slices.Sort(s)
	m := len(s) / 2
	if len(s)%2 == 1 {
		return s[m]
	}
	return (s[m-1] + s[m]) / 2
}

// ssearchMinSpan is how long each timed sample runs the query loop, pass after pass. Go's clock on
// Windows moves in steps of about 0.5 ms; 64 ms keeps one step under 1% of every sample.
const ssearchMinSpan = 64 * time.Millisecond

// timePasses runs the query loop until ssearchMinSpan has passed and returns the result of one
// pass, the elapsed time and the number of passes.
func timePasses(loop queryLoop, a, queries []int64) (h, t uint64, elapsed time.Duration, passes int) {
	start := time.Now()
	for {
		h, t = loop(a, queries)
		passes++
		if elapsed = time.Since(start); elapsed >= ssearchMinSpan {
			return h, t, elapsed, passes
		}
	}
}

func ssearchKernel(w ssearchWay) workKernel {
	return func(c config) (string, []float64, map[string]any, error) {
		times := make([]float64, c.repeats)
		extra := map[string]any{}
		var hits, total uint64
		for _, n := range ssearchSizes {
			rng := newRNG()
			a := make([]int64, n)
			var v int64
			for i := range a {
				v += 1 + int64(rng.Next()%4)
				a[i] = v
			}
			queries := make([]int64, c.size)
			for i := range queries {
				queries[i] = int64(rng.Next() % uint32(v+1))
			}
			if w.same && len(queries) > 2 {
				queries = slices.Repeat([]int64{queries[2]}, len(queries))
			}
			ns := make([]float64, c.repeats)
			var h, t uint64
			for r := range times {
				var elapsed time.Duration
				var passes int
				h, t, elapsed, passes = timePasses(w.loop, a, queries)
				perPass := float64(elapsed.Nanoseconds()) / float64(passes)
				times[r] += perPass / 1e6
				ns[r] = perPass / float64(c.size)
			}
			hits += h
			total += t
			extra[fmt.Sprintf("ns_%d", n)] = math.Round(medianOf(ns)*1e3) / 1e3
		}
		for i := range times {
			times[i] = math.Round(times[i]*1e3) / 1e3
		}
		return fmt.Sprintf("%d:%d", hits, total), times, extra, nil
	}
}
