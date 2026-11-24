// Race: pi-digits (go)
// Extracted from suite/go/puzzle.go — build and run via suite/run.py

package main
import (
	"fmt"
	"math"
	"math/big"
	"math/bits"
)

const (
	chudA         = 13591409
	chudB         = 545140134
	digitsPerTerm = 14
	guardDigits   = 10
)

// chudC324 is 640320^3 / 24.
var chudC324 = big.NewInt(10939058860032000)

// binarySplit returns P(a,b), Q(a,b), T(a,b) of the Chudnovsky terms a .. b-1.
func binarySplit(a, b int64) (p, q, t *big.Int) {
	if b-a == 1 {
		if a == 0 {
			p, q = big.NewInt(1), big.NewInt(1)
		} else {
			p = big.NewInt((6*a - 5) * (2*a - 1) * (6*a - 1))
			q = new(big.Int).Mul(big.NewInt(a*a*a), chudC324)
		}
		t = new(big.Int).Mul(p, big.NewInt(chudA+chudB*a))
		if a%2 == 1 {
			t.Neg(t)
		}
		return p, q, t
	}
	m := (a + b) / 2
	p1, q1, t1 := binarySplit(a, m)
	p2, q2, t2 := binarySplit(m, b)
	t = new(big.Int).Mul(t1, q2)
	t.Add(t, new(big.Int).Mul(p1, t2))
	return new(big.Int).Mul(p1, p2), new(big.Int).Mul(q1, q2), t
}

func powerOfTen(k int) *big.Int {
	return new(big.Int).Exp(big.NewInt(10), big.NewInt(int64(k)), nil)
}

// isqrt is the floor square root of n > 0 by precision doubling (CPython's math.isqrt), written out as in the others.
func isqrt(n *big.Int) *big.Int {
	c := (n.BitLen() - 1) / 2
	a := big.NewInt(1)
	d := 0
	for s := bits.Len(uint(c)) - 1; s >= 0; s-- {
		e := d
		d = c >> s
		q := new(big.Int).Rsh(n, uint(2*c-e-d+1))
		q.Quo(q, a)
		a.Lsh(a, uint(d-e-1)).Add(a, q)
	}
	if new(big.Int).Mul(a, a).Cmp(n) > 0 {
		a.Sub(a, big.NewInt(1))
	}
	return a
}

func piDigits(n int) string {
	terms := int64(n/digitsPerTerm + 2)
	_, q, t := binarySplit(0, terms)
	one := powerOfTen(n + guardDigits)
	sqrtC := new(big.Int).Mul(one, one)
	sqrtC = isqrt(sqrtC.Mul(sqrtC, big.NewInt(10005)))
	pi := new(big.Int).Mul(big.NewInt(426880), sqrtC)
	pi.Mul(pi, q)
	pi.Quo(pi, t)
	pi.Quo(pi, powerOfTen(guardDigits))
	return pi.String()
}

func piChecksum(digits string) string {
	sum := 0
	for _, ch := range digits {
		sum += int(ch - '0')
	}
	return fmt.Sprintf("%d:%s:%d", len(digits)-1, digits[len(digits)-50:], sum)
}
