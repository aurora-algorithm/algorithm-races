package main

// Autocomplete kernels (../SPEC.md section 15): the first 10 words that start with a typed prefix, from a sorted
// list with binary search, a 26-slot trie, or the same trie in one arena slice.

import (
	"fmt"
	"math"
	"os"
	"path/filepath"
	"slices"
	"strings"
	"time"
)

const (
	wordsFile = "words-web2.txt"
	topK      = 10
)

var acKernels = map[string]workKernel{
	"ac-sorted-build":     acSortedBuildKernel,
	"ac-trie-build":       acTrieBuildKernel,
	"ac-trie-arena-build": acTrieArenaBuildKernel,
	"ac-sorted":           acQueryKernel(newSortedCompleter),
	"ac-trie":             acQueryKernel(newTrieCompleter),
	"ac-trie-arena":       acQueryKernel(newArenaCompleter),
}

func shuffledWords(data string) ([]string, *XorShift32, error) {
	text, err := os.ReadFile(filepath.Join(data, wordsFile))
	if err != nil {
		return nil, nil, err
	}
	var words []string
	for line := range strings.SplitSeq(string(text), "\n") {
		if w := strings.TrimSuffix(line, "\r"); w != "" {
			words = append(words, w)
		}
	}
	rng := newRNG()
	for i := len(words) - 1; i > 0; i-- {
		j := rng.Next() % uint32(i+1)
		words[i], words[j] = words[j], words[i]
	}
	return words, rng, nil
}

func buildInput(c config) ([]string, error) {
	words, _, err := shuffledWords(c.data)
	if err != nil {
		return nil, err
	}
	if c.size > len(words) {
		return nil, fmt.Errorf("--size %d is more than the %d words in %s", c.size, len(words), wordsFile)
	}
	return words[:c.size], nil
}

func queryInput(c config) (words, queries []string, err error) {
	words, rng, err := shuffledWords(c.data)
	if err != nil {
		return nil, nil, err
	}
	w := uint32(len(words))
	queries = make([]string, c.size)
	for i := range queries {
		word := words[rng.Next()%w]
		queries[i] = word[:min(2+int(rng.Next()%4), len(word))]
	}
	return words, queries, nil
}

func fold(h uint32, w string) uint32 {
	h = h*31 + uint32(len(w))
	return h*31 + uint32(w[len(w)-1])
}

func buildChecksum(inOrder []string) string {
	var h uint32
	for _, w := range inOrder {
		h = fold(h, w)
	}
	return fmt.Sprintf("%d:%d", len(inOrder), h)
}

// measureBuild times each build like measure, but drops the previous repeat's structure before the next
// repeat's input is made, so the timed region never starts with an old one still alive.
func measureBuild[In, Out any](repeats int, fresh func() In, build func(In) Out) (Out, []float64) {
	var last Out
	times := make([]float64, 0, repeats)
	for range repeats {
		var none Out
		last = none
		in := fresh()
		start := time.Now()
		last = build(in)
		elapsed := float64(time.Since(start).Nanoseconds()) / 1e6
		times = append(times, math.Round(elapsed*1000)/1000)
	}
	return last, times
}

func sortedBuild(a []string) []string {
	slices.Sort(a)
	return a
}

func sortedComplete(a []string, prefix string, out []string) []string {
	i, _ := slices.BinarySearch(a, prefix)
	end := min(i+topK, len(a))
	for ; i < end && strings.HasPrefix(a[i], prefix); i++ {
		out = append(out, a[i])
	}
	return out
}

type acNode struct {
	children [26]*acNode
	word     int32
}

func newACNode() *acNode {
	return &acNode{word: -1}
}

type acTrie struct {
	root  *acNode
	nodes int
}

func trieBuild(words []string) acTrie {
	t := acTrie{root: newACNode(), nodes: 1}
	for idx, w := range words {
		n := t.root
		for i := range len(w) {
			ch := w[i] - 'a'
			if n.children[ch] == nil {
				n.children[ch] = newACNode()
				t.nodes++
			}
			n = n.children[ch]
		}
		n.word = int32(idx)
	}
	return t
}

func collect(n *acNode, words, out []string, limit int) []string {
	if n.word >= 0 {
		out = append(out, words[n.word])
	}
	for _, child := range n.children {
		if len(out) >= limit {
			return out
		}
		if child != nil {
			out = collect(child, words, out, limit)
		}
	}
	return out
}

func trieComplete(root *acNode, words []string, prefix string, out []string) []string {
	n := root
	for i := range len(prefix) {
		n = n.children[prefix[i]-'a']
		if n == nil {
			return out
		}
	}
	return collect(n, words, out, topK)
}

// Node 0 is the root and never anyone's child, so a child link of 0 means no child.
type acArenaNode struct {
	children [26]uint32
	word     int32
}

func arenaBuild(words []string) []acArenaNode {
	nodes := make([]acArenaNode, 1, firstCapacity)
	nodes[0].word = -1
	for idx, w := range words {
		n := uint32(0)
		for i := range len(w) {
			ch := w[i] - 'a'
			if nodes[n].children[ch] == 0 {
				nodes[n].children[ch] = uint32(len(nodes))
				nodes = append(nodes, acArenaNode{word: -1})
			}
			n = nodes[n].children[ch]
		}
		nodes[n].word = int32(idx)
	}
	return nodes
}

func arenaCollect(nodes []acArenaNode, n uint32, words, out []string, limit int) []string {
	if nodes[n].word >= 0 {
		out = append(out, words[nodes[n].word])
	}
	for _, child := range nodes[n].children {
		if len(out) >= limit {
			return out
		}
		if child != 0 {
			out = arenaCollect(nodes, child, words, out, limit)
		}
	}
	return out
}

func arenaComplete(nodes []acArenaNode, words []string, prefix string, out []string) []string {
	n := uint32(0)
	for i := range len(prefix) {
		n = nodes[n].children[prefix[i]-'a']
		if n == 0 {
			return out
		}
	}
	return arenaCollect(nodes, n, words, out, topK)
}

func acSortedBuildKernel(c config) (string, []float64, map[string]any, error) {
	words, err := buildInput(c)
	if err != nil {
		return "", nil, nil, err
	}
	sorted, times := measureBuild(c.repeats, func() []string { return slices.Clone(words) }, sortedBuild)
	return buildChecksum(sorted), times, nil, nil
}

func acTrieBuildKernel(c config) (string, []float64, map[string]any, error) {
	words, err := buildInput(c)
	if err != nil {
		return "", nil, nil, err
	}
	t, times := measureBuild(c.repeats, same(words), trieBuild)
	all := collect(t.root, words, nil, len(words)+1)
	return buildChecksum(all), times, map[string]any{"nodes": t.nodes}, nil
}

func acTrieArenaBuildKernel(c config) (string, []float64, map[string]any, error) {
	words, err := buildInput(c)
	if err != nil {
		return "", nil, nil, err
	}
	nodes, times := measureBuild(c.repeats, same(words), arenaBuild)
	all := arenaCollect(nodes, 0, words, nil, len(words)+1)
	return buildChecksum(all), times, map[string]any{"nodes": len(nodes)}, nil
}

// A completer appends the matches of one prefix to out and returns it.
type completer func(prefix string, out []string) []string

func newSortedCompleter(words []string) completer {
	a := sortedBuild(slices.Clone(words))
	return func(prefix string, out []string) []string { return sortedComplete(a, prefix, out) }
}

func newTrieCompleter(words []string) completer {
	root := trieBuild(words).root
	return func(prefix string, out []string) []string { return trieComplete(root, words, prefix, out) }
}

func newArenaCompleter(words []string) completer {
	nodes := arenaBuild(words)
	return func(prefix string, out []string) []string { return arenaComplete(nodes, words, prefix, out) }
}

type queryResult struct {
	results int
	h       uint32
}

func runQueries(complete completer, queries []string) queryResult {
	var r queryResult
	out := make([]string, 0, topK)
	for _, q := range queries {
		out = complete(q, out[:0])
		r.results += len(out)
		for _, w := range out {
			r.h = fold(r.h, w)
		}
	}
	return r
}

func acQueryKernel(build func(words []string) completer) workKernel {
	return func(c config) (string, []float64, map[string]any, error) {
		words, queries, err := queryInput(c)
		if err != nil {
			return "", nil, nil, err
		}
		complete := build(words)
		r, times := measure(c.repeats, same(queries), func(q []string) queryResult { return runQueries(complete, q) })
		return fmt.Sprintf("%d:%d:%d", len(queries), r.results, r.h), times, nil, nil
	}
}
