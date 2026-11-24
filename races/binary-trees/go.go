// Race: binary-trees (go)
// Extracted from suite/go/bg.go — build and run via suite/run.py

package main
import (
	"fmt"
	"strings"
)

const minDepth = 4

type Node struct {
	Left, Right *Node
}

func makeTree(depth int) *Node {
	if depth == 0 {
		return &Node{}
	}
	return &Node{Left: makeTree(depth - 1), Right: makeTree(depth - 1)}
}

func (n *Node) Check() int {
	if n.Left == nil {
		return 1
	}
	return 1 + n.Left.Check() + n.Right.Check()
}

func depthChecks(depth, iterations int) int {
	check := 0
	for range iterations {
		check += makeTree(depth).Check()
	}
	return check
}

func binaryTreesLines(n int, depthRun func(depth, iterations int) int) string {
	maxDepth := max(minDepth+2, n)
	stretch := maxDepth + 1
	var sb strings.Builder
	fmt.Fprintf(&sb, "stretch tree of depth %d\t check: %d\n", stretch, makeTree(stretch).Check())
	longLived := makeTree(maxDepth)
	for depth := minDepth; depth <= maxDepth; depth += 2 {
		iterations := 1 << (maxDepth - depth + minDepth)
		fmt.Fprintf(&sb, "%d\t trees of depth %d\t check: %d\n", iterations, depth, depthRun(depth, iterations))
	}
	fmt.Fprintf(&sb, "long lived tree of depth %d\t check: %d\n", maxDepth, longLived.Check())
	return sb.String()
}

func binaryTrees(n int) string {
	return binaryTreesLines(n, depthChecks)
}
