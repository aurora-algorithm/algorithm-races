package main

import (
	"encoding/binary"
	"fmt"
	"strings"
)

const (
	seed       = 2463534242
	fnvOffset  = 2166136261
	fnvPrime   = 16777619
	shaBlockMB = 1 << 20
)

// XorShift32 is Marsaglia's xorshift generator; every input in SPEC section 2 comes from a fresh one.
type XorShift32 struct {
	s uint32
}

func newRNG() *XorShift32 {
	return &XorShift32{s: seed}
}

func (r *XorShift32) Next() uint32 {
	r.s ^= r.s << 13
	r.s ^= r.s >> 17
	r.s ^= r.s << 5
	return r.s
}

func (r *XorShift32) Ints(n int) []int64 {
	out := make([]int64, n)
	for i := range out {
		out[i] = int64(r.Next())
	}
	return out
}

func ints(n int) []int64 {
	return newRNG().Ints(n)
}

// ints64 uses all 64 bits: two draws per value, high word first, read as a signed int64.
func ints64(n int) []int64 {
	r := newRNG()
	out := make([]int64, n)
	for i := range out {
		hi := uint64(r.Next())
		lo := uint64(r.Next())
		out[i] = int64(hi<<32 | lo)
	}
	return out
}

func h31[T int64 | float64](values []T) uint32 {
	var h uint32
	for _, v := range values {
		h = h*31 + uint32(v)
	}
	return h
}

func fnv1a(data []byte) uint32 {
	h := uint32(fnvOffset)
	for _, b := range data {
		h = (h ^ uint32(b)) * fnvPrime
	}
	return h
}

func randomStrings(n int) []string {
	rng := newRNG()
	out := make([]string, n)
	for i := range out {
		length := 8 + rng.Next()%9
		s := make([]byte, length)
		for j := range s {
			s[j] = byte('a' + rng.Next()%26)
		}
		out[i] = string(s)
	}
	return out
}

func shaBuffer(size int) []byte {
	block := make([]byte, 0, shaBlockMB)
	for _, v := range newRNG().Ints(shaBlockMB / 4) {
		block = binary.LittleEndian.AppendUint32(block, uint32(v))
	}
	buf := make([]byte, 0, size)
	for range size / shaBlockMB {
		buf = append(buf, block...)
	}
	return buf
}

var (
	methods  = []string{"GET", "POST", "PUT", "DELETE"}
	statuses = []int{200, 201, 204, 301, 304, 400, 401, 403, 404, 500, 502, 503}
)

func logText(lines int) string {
	rng := newRNG()
	var sb strings.Builder
	for range lines {
		method := methods[rng.Next()%4]
		version := 1 + rng.Next()%3
		user := rng.Next() % 100000
		status := statuses[rng.Next()%12]
		ms := rng.Next() % 1000
		fmt.Fprintf(&sb, "%s /api/v%d/users/%d %d %dms\n", method, version, user, status, ms)
	}
	return sb.String()
}

func matrices(n int) (a, b []float64) {
	rng := newRNG()
	fill := func() []float64 {
		m := make([]float64, n*n)
		for i := range m {
			m[i] = float64(rng.Next() % 10)
		}
		return m
	}
	a = fill()
	b = fill()
	return a, b
}
