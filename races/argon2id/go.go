// Race: argon2id (go)
// Extracted from suite/go/work.go — build and run via suite/run.py

package main
import (
	"cmp"
	"crypto/rand"
	"crypto/subtle"
	"encoding/base64"
	"errors"
	"fmt"
	"strings"
	"golang.org/x/crypto/argon2"
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

func b2i(b bool) int {
	if b {
		return 1
	}
	return 0
}

var b64 = base64.RawStdEncoding

func argon2Prefix() string {
	return fmt.Sprintf("$argon2id$v=%d$m=%d,t=%d,p=%d$", argon2.Version, argon2Memory, argon2Time, argon2Lanes)
}

// argon2Hash returns the PHC string (x/crypto/argon2 only derives the key; the encoding is ours).
func argon2Hash(pw string) (string, error) {
	salt := make([]byte, 16)
	if _, err := rand.Read(salt); err != nil {
		return "", err
	}
	key := argon2.IDKey([]byte(pw), salt, argon2Time, argon2Memory, argon2Lanes, argon2KeyLen)
	return argon2Prefix() + b64.EncodeToString(salt) + "$" + b64.EncodeToString(key), nil
}

func argon2Verifies(phc, pw string) bool {
	parts := strings.Split(phc, "$")
	if len(parts) != 6 || parts[1] != "argon2id" || parts[2] != "v=19" {
		return false
	}
	var m, t uint32
	var p uint8
	if _, err := fmt.Sscanf(parts[3], "m=%d,t=%d,p=%d", &m, &t, &p); err != nil {
		return false
	}
	salt, err1 := b64.DecodeString(parts[4])
	want, err2 := b64.DecodeString(parts[5])
	if err := errors.Join(err1, err2); err != nil {
		return false
	}
	got := argon2.IDKey([]byte(pw), salt, t, m, p, uint32(len(want)))
	return subtle.ConstantTimeCompare(got, want) == 1
}

func argon2Checksum(hashes []string) string {
	ok := 0
	for i, h := range hashes {
		if strings.HasPrefix(h, argon2Prefix()) && argon2Verifies(h, fmt.Sprintf("user-%d-password", i)) {
			ok++
		}
	}
	return fmt.Sprintf("%d:%d:%d", len(hashes), ok, b2i(argon2Verifies(argon2Ref, refPassword)))
}
