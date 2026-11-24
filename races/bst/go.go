// Race: bst (go)
// Extracted from suite/go/algo.go — build and run via suite/run.py

package main
import (
	"fmt"
)

type node struct {
	key         int64
	count       int
	left, right *node
}

func bst(in hashmapInput) string {
	var root *node
	size := 0
	for _, k := range in.inserts {
		link := &root
		for *link != nil && (*link).key != k {
			if k < (*link).key {
				link = &(*link).left
			} else {
				link = &(*link).right
			}
		}
		if *link == nil {
			*link = &node{key: k, count: 1}
			size++
		} else {
			(*link).count++
		}
	}
	hits := 0
	for _, k := range in.lookups {
		n := root
		for n != nil && n.key != k {
			if k < n.key {
				n = n.left
			} else {
				n = n.right
			}
		}
		if n != nil {
			hits++
		}
	}
	return fmt.Sprintf("%d:%d:%d", size, hits, h31(inOrder(root, size)))
}

func inOrder(root *node, size int) []int64 {
	keys := make([]int64, 0, size)
	var stack []*node
	n := root
	for n != nil || len(stack) > 0 {
		for n != nil {
			stack = append(stack, n)
			n = n.left
		}
		n = stack[len(stack)-1]
		stack = stack[:len(stack)-1]
		keys = append(keys, n.key)
		n = n.right
	}
	return keys
}

func bstKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(newHashmapInput(c.size)), bst)
	return out, times, nil
}
