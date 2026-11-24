// Race: lru (go)
// Extracted from suite/go/algo.go — build and run via suite/run.py

package main
import (
	"container/list"
	"fmt"
)

type lruInput struct {
	capacity int
	keys     []int
}

func lruKeys(n int) lruInput {
	capacity := max(1, n/100)
	rng := newRNG()
	keys := make([]int, n)
	for i := range keys {
		keys[i] = int(rng.Next() % uint32(2*capacity))
	}
	return lruInput{capacity: capacity, keys: keys}
}

// lru keeps the most recent key at the front of a container/list; the map finds a key's element.
func lru(in lruInput) string {
	order := list.New()
	elems := make(map[int]*list.Element, in.capacity)
	hits := 0
	for _, k := range in.keys {
		if e, ok := elems[k]; ok {
			order.MoveToFront(e)
			hits++
			continue
		}
		if order.Len() == in.capacity {
			oldest := order.Back()
			delete(elems, oldest.Value.(int))
			order.Remove(oldest)
		}
		elems[k] = order.PushFront(k)
	}
	keys := make([]int64, 0, order.Len())
	for e := order.Back(); e != nil; e = e.Prev() {
		keys = append(keys, int64(e.Value.(int)))
	}
	return fmt.Sprintf("%d:%d:%d", hits, len(in.keys)-hits, h31(keys))
}

func lruKernel(c config) (string, []float64, error) {
	out, times := measure(c.repeats, same(lruKeys(c.size)), lru)
	return out, times, nil
}
