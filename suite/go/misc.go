package main

import (
	"bytes"
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"regexp"
	"strconv"
)

var logPattern = regexp.MustCompile(`(GET|POST) /api/v[0-9]/users/([0-9]+) (5[0-9][0-9])`)

func regexLog(text string) string {
	count, ids := 0, 0
	for _, m := range logPattern.FindAllStringSubmatch(text, -1) {
		id, _ := strconv.Atoi(m[2]) // the pattern only matches digits
		count++
		ids += id
	}
	return fmt.Sprintf("%d:%d", count, ids)
}

type hashmapInput struct {
	inserts, lookups []int64
}

func newHashmapInput(n int) hashmapInput {
	inserts := ints(n)
	lookups := make([]int64, n)
	for i, k := range inserts {
		lookups[i] = k ^ int64(i&1)
	}
	return hashmapInput{inserts: inserts, lookups: lookups}
}

func hashmap(in hashmapInput) string {
	counts := map[int64]int64{}
	for _, k := range in.inserts {
		counts[k]++
	}
	hits := 0
	for _, k := range in.lookups {
		if _, ok := counts[k]; ok {
			hits++
		}
	}
	return fmt.Sprintf("%d:%d", len(counts), hits)
}

func sha256Hex(buf []byte) string {
	sum := sha256.Sum256(buf)
	return hex.EncodeToString(sum[:])
}

func sieve(n int) string {
	composite := make([]bool, n+1)
	for i := 2; i*i <= n; i++ {
		if !composite[i] {
			for j := i * i; j <= n; j += i {
				composite[j] = true
			}
		}
	}
	count := 0
	for i := 2; i <= n; i++ {
		if !composite[i] {
			count++
		}
	}
	return strconv.Itoa(count)
}

type matrixInput struct {
	a, b []float64
	n    int
}

func matmul(m matrixInput) []float64 {
	n := m.n
	c := make([]float64, n*n)
	for i := range n {
		row := i * n
		for j := range n {
			s := 0.0
			for k := range n {
				s += m.a[row+k] * m.b[k*n+j]
			}
			c[row+j] = s
		}
	}
	return c
}

func matmulChecksum(c []float64) string {
	var total int64
	for _, v := range c {
		total += int64(v)
	}
	return fmt.Sprintf("%d:%d", total, h31(c))
}

type Order struct {
	ID         int64    `json:"id"`
	CustomerID int64    `json:"customer_id"`
	Status     string   `json:"status"`
	TotalCents int64    `json:"total_cents"`
	Items      int64    `json:"items"`
	Tags       []string `json:"tags"`
}

type OrdersDoc struct {
	Orders []Order `json:"orders"`
}

type jsonResult struct {
	orders, totalCents, items int64
	out                       []byte
}

func jsonRoundtrip(text []byte) (jsonResult, error) {
	var doc OrdersDoc
	if err := json.Unmarshal(text, &doc); err != nil {
		return jsonResult{}, fmt.Errorf("parse orders: %w", err)
	}
	var total, items int64
	for _, o := range doc.Orders {
		total += o.TotalCents
		items += o.Items
	}
	out, err := json.Marshal(doc)
	if err != nil {
		return jsonResult{}, fmt.Errorf("serialise orders: %w", err)
	}
	return jsonResult{orders: int64(len(doc.Orders)), totalCents: total, items: items, out: out}, nil
}

var errRoundtrip = errors.New("json output differs from the input")

func jsonChecksum(r jsonResult, input []byte) (string, error) {
	if !bytes.Equal(r.out, input) {
		return "", errRoundtrip
	}
	return fmt.Sprintf("%d:%d:%d:%d:%d", r.orders, r.totalCents, r.items, len(r.out), fnv1a(r.out)), nil
}
