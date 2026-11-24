// Race: sha256 (go)
// Extracted from suite/go/misc.go — build and run via suite/run.py

package main
import (
	"crypto/sha256"
	"encoding/hex"
)

func sha256Hex(buf []byte) string {
	sum := sha256.Sum256(buf)
	return hex.EncodeToString(sum[:])
}
