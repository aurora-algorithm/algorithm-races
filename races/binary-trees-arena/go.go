// Race: binary-trees-arena (go)
// Extracted from suite/go/arena.go — build and run via suite/run.py

package main
import (
	"fmt"
	"strings"
)

// Every arena starts with room for 16 nodes, as in C#, Java and JS, then append grows it.
const firstCapacity = 16

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
