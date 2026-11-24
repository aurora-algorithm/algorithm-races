// Race: trie-arena (go)
// Extracted from suite/go/arena.go — build and run via suite/run.py

package main
import (
	"fmt"
)

// Every arena starts with room for 16 nodes, as in C#, Java and JS, then append grows it.
const firstCapacity = 16

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
