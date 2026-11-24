// Race: bst-arena (go)
// Extracted from suite/go/arena.go — build and run via suite/run.py

package main
import (
	"fmt"
)

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
