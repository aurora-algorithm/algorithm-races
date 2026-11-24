// Race: binary-trees-mt (go)
// Extracted from suite/go/mt.go — build and run via suite/run.py

package main
import "sync"

func binaryTreesMT(n, threads int) string {
	depthRun := func(depth, iterations int) int {
		share := iterations / threads
		checks := make([]int, threads)
		var wg sync.WaitGroup
		for t := range threads {
			wg.Go(func() { checks[t] = depthChecks(depth, share) })
		}
		wg.Wait()
		total := 0
		for _, c := range checks {
			total += c
		}
		return total
	}
	return binaryTreesLines(n, depthRun)
}
