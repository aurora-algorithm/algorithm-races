package main

// The builtins group (../builtins-NOTES.md): the standard library's ways of doing one job, one way per kernel,
// named JOB.go.WAY. Every way of a job gets the same input and prints the same checksum.

import (
	"bytes"
	"cmp"
	"fmt"
	"maps"
	"slices"
	"sort"
	"strconv"
	"strings"

	"github.com/shawnsmithdev/zermelo/v2"
)

func init() {
	intInputs := map[string]func(int) []int64{
		"bsort-int":          ints,
		"bsort-int-sorted":   func(n int) []int64 { return sortedCopy(ints(n), false) },
		"bsort-int-reversed": func(n int) []int64 { return sortedCopy(ints(n), true) },
		"bsort-int64":        ints64,
		"bsort-int64-sorted": func(n int) []int64 { return sortedCopy(ints64(n), false) },
	}
	strInputs := map[string]func(int) []string{
		"bsort-str":          randomStrings,
		"bsort-str-sorted":   func(n int) []string { return sortedCopy(randomStrings(n), false) },
		"bsort-str-reversed": func(n int) []string { return sortedCopy(randomStrings(n), true) },
	}
	intSorts := map[string]func([]int64){
		"slices.Sort": slices.Sort[[]int64],
		"sort.Slice": func(a []int64) {
			sort.Slice(a, func(i, j int) bool { return a[i] < a[j] })
		},
		"slices.SortFunc": func(a []int64) { slices.SortFunc(a, cmp.Compare[int64]) },
		// Library way: zermelo v2.1.0, an LSD radix sort (allocates its buffer inside the call, so that is timed).
		"zermelo.Sort": zermelo.Sort[int64],
	}
	strSorts := map[string]func([]string){
		"slices.Sort":  slices.Sort[[]string],
		"sort.Strings": sort.Strings,
		"sort.Slice": func(a []string) {
			sort.Slice(a, func(i, j int) bool { return a[i] < a[j] })
		},
		"slices.SortFunc": func(a []string) { slices.SortFunc(a, strings.Compare) },
	}
	ways := map[string]kernel{}
	for job, input := range intInputs {
		for way, run := range intSorts {
			ways[job+".go."+way] = waySort(input, run, sortedChecksum)
		}
		ways[job+".go.sort.Ints"] = sortIntsKernel(input)
	}
	for job, input := range strInputs {
		for way, run := range strSorts {
			ways[job+".go."+way] = waySort(input, run, stringsChecksum)
		}
	}
	ways["bmap-int.go.map"] = mapKernel(mapIntInput)
	ways["bmap-str.go.map"] = mapStrKernel
	for way, join := range map[string]func([]string) string{
		"strings.Join":         func(p []string) string { return strings.Join(p, ",") },
		"strings.Builder":      joinBuilder,
		"strings.Builder.Grow": joinBuilderGrow,
		"bytes.Buffer":         joinBuffer,
	} {
		ways["bjoin.go."+way] = joinKernel(join)
	}
	maps.Copy(kernels, ways)
}

func sortedCopy[T cmp.Ordered](a []T, reverse bool) []T {
	slices.Sort(a)
	if reverse {
		slices.Reverse(a)
	}
	return a
}

func waySort[T any](input func(int) []T, run func([]T), check func([]T) string) kernel {
	return func(c config) (string, []float64, error) {
		base := input(c.size)
		sorted := func(a []T) []T {
			run(a)
			return a
		}
		out, times := measure(c.repeats, func() []T { return slices.Clone(base) }, sorted)
		return check(out), times, nil
	}
}

// sortIntsKernel: sort.Ints takes []int (64-bit on amd64 and arm64); the conversion from []int64 is untimed.
func sortIntsKernel(input func(int) []int64) kernel {
	return func(c config) (string, []float64, error) {
		base := input(c.size)
		fresh := func() []int {
			a := make([]int, len(base))
			for i, v := range base {
				a[i] = int(v)
			}
			return a
		}
		sorted := func(a []int) []int {
			sort.Ints(a)
			return a
		}
		out, times := measure(c.repeats, fresh, sorted)
		back := make([]int64, len(out))
		for i, v := range out {
			back[i] = int64(v)
		}
		return sortedChecksum(back), times, nil
	}
}

type mapInput[K comparable] struct {
	inserts, lookups []K
}

func mapIntInput(n int) mapInput[int64] {
	inserts := ints(n)
	lookups := make([]int64, n)
	for i, k := range inserts {
		lookups[i] = k ^ int64(i&1)
	}
	return mapInput[int64]{inserts, lookups}
}

// mapStrInput: even i looks up a key in the map, odd i the key without its last letter (almost never in it).
func mapStrInput(n int) mapInput[string] {
	inserts := randomStrings(n)
	lookups := make([]string, n)
	for i, k := range inserts {
		if i%2 == 0 {
			lookups[i] = k
		} else {
			lookups[i] = k[:len(k)-1]
		}
	}
	return mapInput[string]{inserts, lookups}
}

func countMap[K comparable](in mapInput[K]) string {
	counts := map[K]int64{}
	for _, k := range in.inserts {
		counts[k]++
	}
	hits := 0
	for _, k := range in.lookups {
		if _, ok := counts[k]; ok {
			hits++
		}
	}
	return fmt.Sprintf("%d:%d", len(counts), hits)
}

func mapKernel[K comparable](input func(int) mapInput[K]) kernel {
	return func(c config) (string, []float64, error) {
		out, times := measure(c.repeats, same(input(c.size)), countMap[K])
		return out, times, nil
	}
}

// mapStrKernel gives every repeat its own copies of the inserts and, separately, of the lookups (made untimed), as
// the languages whose strings cache their hash need; equal keys are never the same pointer.
func mapStrKernel(c config) (string, []float64, error) {
	in := mapStrInput(c.size)
	fresh := func() mapInput[string] {
		return mapInput[string]{cloneAll(in.inserts), cloneAll(in.lookups)}
	}
	out, times := measure(c.repeats, fresh, countMap[string])
	return out, times, nil
}

func cloneAll(keys []string) []string {
	out := make([]string, len(keys))
	for i, k := range keys {
		out[i] = strings.Clone(k)
	}
	return out
}

func joinParts(n int) []string {
	values := ints(n)
	parts := make([]string, n)
	for i, v := range values {
		parts[i] = strconv.FormatInt(v, 10)
	}
	return parts
}

func joinBuilder(parts []string) string {
	var b strings.Builder
	for i, p := range parts {
		if i > 0 {
			b.WriteByte(',')
		}
		b.WriteString(p)
	}
	return b.String()
}

func joinBuilderGrow(parts []string) string {
	total := max(len(parts)-1, 0)
	for _, p := range parts {
		total += len(p)
	}
	var b strings.Builder
	b.Grow(total)
	for i, p := range parts {
		if i > 0 {
			b.WriteByte(',')
		}
		b.WriteString(p)
	}
	return b.String()
}

func joinBuffer(parts []string) string {
	var b bytes.Buffer
	for i, p := range parts {
		if i > 0 {
			b.WriteByte(',')
		}
		b.WriteString(p)
	}
	return b.String()
}

func joinKernel(join func([]string) string) kernel {
	return func(c config) (string, []float64, error) {
		out, times := measure(c.repeats, same(joinParts(c.size)), join)
		return fmt.Sprintf("%d:%d", len(out), fnv1a([]byte(out))), times, nil
	}
}
