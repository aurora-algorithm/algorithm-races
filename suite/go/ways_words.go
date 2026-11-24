package main

// The builtins group's many-words jobs (../builtins-NOTES.md, "Many words"): find every place any of K words occurs
// in a log, one way per kernel named bwords-K.go.WAY. Every way prints "{K}:{matches}:{sum of match start offsets}";
// the words are 8-digit ids hidden inside longer digit runs (session tokens), and no two matches overlap in these logs.

import (
	"fmt"
	"maps"
	"regexp"
	"strconv"
	"strings"

	ahocorasick "github.com/petar-dambovaliev/aho-corasick"
)

const wordPool = 10000

type wordsInput struct {
	text  string
	words []string
}

func init() {
	ways := map[string]kernel{}
	for _, k := range []int{1, 10, 100, 1000} {
		for way, search := range map[string]func(string, []string) [2]int{
			"strings.Index": indexLoop,
			"regexp":        regexpAlternation,
			"aho-corasick":  ahoCorasickLib,
		} {
			ways[fmt.Sprintf("bwords-%d.go.%s", k, way)] = wordsKernel(k, search)
		}
	}
	maps.Copy(kernels, ways)
}

func wordsKernel(k int, search func(string, []string) [2]int) kernel {
	return func(c config) (string, []float64, error) {
		text, users := wordsLog(c.size)
		in := wordsInput{text, firstDistinct(users, k)}
		out, times := measure(c.repeats, same(in), func(w wordsInput) [2]int { return search(w.text, w.words) })
		return fmt.Sprintf("%d:%d:%d", k, out[0], out[1]), times, nil
	}
}

// randomDigits returns 0 to 6 random digits: the length first, then each digit.
func randomDigits(rng *XorShift32) string {
	n := rng.Next() % 7
	b := make([]byte, n)
	for i := range b {
		b[i] = byte('0' + rng.Next()%10)
	}
	return string(b)
}

func wordsLog(lines int) (string, []string) {
	rng := newRNG()
	pool := make([]string, wordPool)
	for i := range pool {
		pool[i] = strconv.FormatUint(uint64(10000000+rng.Next()%90000000), 10)
	}
	var sb strings.Builder
	users := make([]string, 0, lines)
	for range lines {
		method := methods[rng.Next()%4]
		version := 1 + rng.Next()%3
		user := pool[rng.Next()%wordPool]
		users = append(users, user)
		before := randomDigits(rng)
		after := randomDigits(rng)
		status := statuses[rng.Next()%12]
		ms := rng.Next() % 1000
		fmt.Fprintf(&sb, "%s /api/v%d/cart?sid=%s%s%s %d %dms\n", method, version, before, user, after, status, ms)
	}
	return sb.String(), users
}

// firstDistinct returns the words: the ids of the log's lines in line order, repeats skipped, the first k.
func firstDistinct(users []string, k int) []string {
	seen := map[string]bool{}
	words := make([]string, 0, k)
	for _, w := range users {
		if len(words) == k {
			break
		}
		if !seen[w] {
			seen[w] = true
			words = append(words, w)
		}
	}
	return words
}

func indexLoop(text string, words []string) [2]int {
	var r [2]int
	for _, w := range words {
		from := 0
		for {
			i := strings.Index(text[from:], w)
			if i < 0 {
				break
			}
			r[0]++
			r[1] += from + i
			from += i + 1
		}
	}
	return r
}

func regexpAlternation(text string, words []string) [2]int {
	pattern := regexp.MustCompile(strings.Join(words, "|"))
	var r [2]int
	for _, m := range pattern.FindAllStringIndex(text, -1) {
		r[0]++
		r[1] += m[0]
	}
	return r
}

// The options are the ones the library's README shows.
func ahoCorasickLib(text string, words []string) [2]int {
	builder := ahocorasick.NewAhoCorasickBuilder(ahocorasick.Opts{
		AsciiCaseInsensitive: false,
		MatchOnlyWholeWords:  false,
		MatchKind:            ahocorasick.LeftMostLongestMatch,
		DFA:                  true,
	})
	automaton := builder.Build(words)
	var r [2]int
	for _, m := range automaton.FindAll(text) {
		r[0]++
		r[1] += m.Start()
	}
	return r
}
