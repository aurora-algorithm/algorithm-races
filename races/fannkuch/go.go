// Race: fannkuch (go)
// Extracted from suite/go/bg.go — build and run via suite/run.py

package main
import (
	"fmt"
	"slices"
)

func fannkuch(n int) string {
	perm1 := make([]int, n)
	for i := range perm1 {
		perm1[i] = i
	}
	perm := make([]int, n)
	count := make([]int, n)
	maxFlips, checksum, permCount := 0, 0, 0
	r := n
	for {
		for ; r != 1; r-- {
			count[r-1] = r
		}
		copy(perm, perm1)
		flips := 0
		for k := perm[0]; k != 0; k = perm[0] {
			slices.Reverse(perm[:k+1])
			flips++
		}
		maxFlips = max(maxFlips, flips)
		if permCount%2 == 0 {
			checksum += flips
		} else {
			checksum -= flips
		}
		for {
			if r == n {
				return fmt.Sprintf("%d:%d", checksum, maxFlips)
			}
			perm0 := perm1[0]
			copy(perm1[:r], perm1[1:r+1])
			perm1[r] = perm0
			count[r]--
			if count[r] > 0 {
				break
			}
			r++
		}
		permCount++
	}
}
