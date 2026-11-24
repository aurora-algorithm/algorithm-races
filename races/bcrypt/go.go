// Race: bcrypt (go)
// Extracted from suite/go/work.go — build and run via suite/run.py

package main
import (
	"cmp"
	"fmt"
	"golang.org/x/crypto/bcrypt"
)

const (
	photoFiles   = 8
	targetWidth  = 1080
	jpegQuality  = 80
	bcryptCost   = 10
	argon2Memory = 19456
	argon2Time   = 2
	argon2Lanes  = 1
	argon2KeyLen = 32
	refPassword  = "correct horse battery staple"
	bcryptRef    = "$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy"
	argon2Ref    = "$argon2id$v=19$m=19456,t=2,p=1$c3RhY2tzLWJlbmNoLXNhbHQ$oLeQRnXjHhLMJZcTdYDeVr93DW7srzPjTWhPG3BTryk"
)

type workKernel func(c config) (checksum string, timesMs []float64, extra map[string]any, err error)

func passwords(n int) []string {
	pws := make([]string, n)
	for i := range pws {
		pws[i] = fmt.Sprintf("user-%d-password", i)
	}
	return pws
}

func passwordKernel(hash func(string) (string, error), check func([]string) string) workKernel {
	return func(c config) (string, []float64, map[string]any, error) {
		var runErr error
		hashAll := func(pws []string) []string {
			out := make([]string, len(pws))
			for i, pw := range pws {
				h, err := hash(pw)
				runErr = cmp.Or(runErr, err)
				out[i] = h
			}
			return out
		}
		hashes, times := measure(c.repeats, same(passwords(c.size)), hashAll)
		if runErr != nil {
			return "", nil, nil, runErr
		}
		return check(hashes), times, nil, nil
	}
}

func bcryptHash(pw string) (string, error) {
	h, err := bcrypt.GenerateFromPassword([]byte(pw), bcryptCost)
	return string(h), err
}

func bcryptVerifies(hash, pw string) bool {
	return bcrypt.CompareHashAndPassword([]byte(hash), []byte(pw)) == nil
}

func bcryptChecksum(hashes []string) string {
	ok := 0
	for i, h := range hashes {
		cost, err := bcrypt.Cost([]byte(h))
		if err == nil && cost == bcryptCost && bcryptVerifies(h, fmt.Sprintf("user-%d-password", i)) {
			ok++
		}
	}
	return fmt.Sprintf("%d:%d:%d", len(hashes), ok, b2i(bcryptVerifies(bcryptRef, refPassword)))
}

func b2i(b bool) int {
	if b {
		return 1
	}
	return 0
}
