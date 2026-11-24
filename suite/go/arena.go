package main

// The -arena kernels (../SPEC.md section 12, "Arena rows"): bst, trie and binary-trees with every node in one
// growable slice and 32-bit index links, the same algorithms and checksums as the pointer versions.

import (
	"fmt"
	"maps"
	"strings"
)

func init() {
	maps.Copy(kernels, map[string]kernel{
		"bst-arena":          bstArenaKernel,
		"trie-arena":         trieArenaKernel,
		"binary-trees-arena": sizeKernel(binaryTreesArena, textChecksum),
	})
}

// Every arena starts with room for 16 nodes, as in C#, Java and JS, then append grows it.
const firstCapacity = 16

// A link of -1 means no child.
type arenaNode struct {
	key         int64
	count       int32
	left, right int32
}

func bstArena(in hashmapInput) string {
	nodes := make([]arenaNode, 0, firstCapacity)
	for _, k := range in.inserts {
		if len(nodes) == 0 {
			nodes = append(nodes, arenaNode{key: k, count: 1, left: -1, right: -1})
			continue
		}
		link := &nodes[0].left
		cur := int32(0)
		for cur >= 0 && nodes[cur].key != k {
			if k < nodes[cur].key {
				link = &nodes[cur].left
			} else {
				link = &nodes[cur].right
			}
			cur = *link
		}
		if cur < 0 {
			*link = int32(len(nodes))
			nodes = append(nodes, arenaNode{key: k, count: 1, left: -1, right: -1})
		} else {
			nodes[cur].count++
		}
	}
	hits := 0
	for _, k := range in.lookups {
		cur := int32(-1)
		if len(nodes) > 0 {
			cur = 0
		}
		for cur >= 0 && nodes[cur].key != k {
			if k < nodes[cur].key {
				cur = nodes[cur].left
			} else {
				cur = nodes[cur].right
			}
		}
		if cur >= 0 {
			hits++
		}
	}
	return fmt.Sprintf("%d:%d:%d", len(nodes), hits, h31(inOrderArena(nodes)))
}

func inOrderArena(nodes []arenaNode) []int64 {
	keys := make([]int64, 0, len(nodes))
	var stack []int32
	n := int32(-1)
	if len(nodes) > 0 {
		n = 0
	}
	for n >= 0 || len(stack) > 0 {
		for n >= 0 {
			stack = append(stack, n)
			n = nodes[n].left
		}
		n = stack[len(stack)-1]
		stack = stack[:len(stack)-1]
		keys = append(keys, nodes[n].key)
		n = nodes[n].right
	}
	return keys
}

func bstArenaKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(newHashmapInput(c.size)), bstArena)
	return out, times, nil
}

// Node 0 is the root and never anyone's child, so a child link of 0 means no child.
type trieArenaNode struct {
	count    int32
	children [26]int32
}

func trieArena(in trieInput) string {
	nodes := make([]trieArenaNode, 1, firstCapacity)
	for _, w := range in.inserted {
		n := int32(0)
		for i := range len(w) {
			ch := w[i] - 'a'
			if nodes[n].children[ch] == 0 {
				nodes[n].children[ch] = int32(len(nodes))
				nodes = append(nodes, trieArenaNode{})
			}
			n = nodes[n].children[ch]
			nodes[n].count++
		}
	}
	total := 0
	for _, p := range in.prefixes {
		// A prefix has at least one letter, so n == 0 after the walk means it is absent.
		n := int32(0)
		for i := 0; i < len(p); i++ {
			n = nodes[n].children[p[i]-'a']
			if n == 0 {
				break
			}
		}
		if n != 0 {
			total += int(nodes[n].count)
		}
	}
	return fmt.Sprintf("%d:%d", len(nodes)-1, total)
}

func trieArenaKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(words(c.size)), trieArena)
	return out, times, nil
}

// A tree's nodes in one slice, children pushed before their parent; a leaf's links are -1.
type treeArenaNode struct {
	left, right int32
}

type treeArena struct {
	nodes []treeArenaNode
}

func (t *treeArena) make(depth int) int32 {
	node := treeArenaNode{left: -1, right: -1}
	if depth > 0 {
		node.left = t.make(depth - 1)
		node.right = t.make(depth - 1)
	}
	t.nodes = append(t.nodes, node)
	return int32(len(t.nodes) - 1)
}

func (t *treeArena) check(n int32) int {
	node := t.nodes[n]
	if node.left < 0 {
		return 1
	}
	return 1 + t.check(node.left) + t.check(node.right)
}

func newTreeArena() treeArena {
	return treeArena{nodes: make([]treeArenaNode, 0, firstCapacity)}
}

func treeArenaCheck(depth int) int {
	t := newTreeArena()
	return t.check(t.make(depth))
}

func binaryTreesArena(n int) string {
	maxDepth := max(minDepth+2, n)
	stretch := maxDepth + 1
	var sb strings.Builder
	fmt.Fprintf(&sb, "stretch tree of depth %d\t check: %d\n", stretch, treeArenaCheck(stretch))
	longLived := newTreeArena()
	longRoot := longLived.make(maxDepth)
	for depth := minDepth; depth <= maxDepth; depth += 2 {
		iterations := 1 << (maxDepth - depth + minDepth)
		check := 0
		for range iterations {
			check += treeArenaCheck(depth)
		}
		fmt.Fprintf(&sb, "%d\t trees of depth %d\t check: %d\n", iterations, depth, check)
	}
	fmt.Fprintf(&sb, "long lived tree of depth %d\t check: %d\n", maxDepth, longLived.check(longRoot))
	return sb.String()
}
