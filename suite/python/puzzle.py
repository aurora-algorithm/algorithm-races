"""Unusual algorithm kernels (kernels/SPEC.md section 13): one clever trick each, the same steps in every language."""

import heapq
import math
import sys

from gen import MASK, XorShift32, fnv1a, h31
from sudoku import sudoku, sudoku_puzzles

ONE_MIB = 1 << 20


def fmix32(h: int) -> int:
    """MurmurHash3's 32-bit finalizer: the one hash every language uses for the probabilistic kernels."""
    h ^= h >> 16
    h = (h * 0x85EBCA6B) & MASK
    h ^= h >> 13
    h = (h * 0xC2B2AE35) & MASK
    h ^= h >> 16
    return h


# bloom: k bit positions per key from two hashes

BLOOM_BITS_PER_KEY = 10
BLOOM_HASHES = 7
GOLDEN = 0x9E3779B9


def bloom_input(n: int) -> tuple[list[int], list[int]]:
    values = XorShift32().ints(2 * n)
    return values[:n], values[n:]


def bloom(keys: list[int], lookups: list[int]) -> str:
    m = BLOOM_BITS_PER_KEY * len(keys)
    bits = bytearray((m + 7) // 8)
    for key in keys:
        h1 = fmix32(key)
        h2 = fmix32(key ^ GOLDEN) | 1
        for i in range(BLOOM_HASHES):
            b = ((h1 + i * h2) & MASK) % m
            bits[b >> 3] |= 1 << (b & 7)
    false_positives = 0
    for key in lookups:
        h1 = fmix32(key)
        h2 = fmix32(key ^ GOLDEN) | 1
        for i in range(BLOOM_HASHES):
            b = ((h1 + i * h2) & MASK) % m
            if not bits[b >> 3] & (1 << (b & 7)):
                break
        else:
            false_positives += 1
    set_bits = sum(byte.bit_count() for byte in bits)
    return f"{len(keys)}:{set_bits}:{false_positives}"


# hyperloglog: the longest run of leading zeros estimates how many distinct items went by

HLL_P = 14
HLL_M = 1 << HLL_P


def hyperloglog(n: int) -> str:
    rng = XorShift32()
    universe = max(n // 4, 1)
    registers = bytearray(HLL_M)
    for _ in range(n):
        h = fmix32(rng.next() % universe)
        w = (h << HLL_P) & MASK
        rank = 33 - w.bit_length() if w else 32 - HLL_P + 1
        idx = h >> (32 - HLL_P)
        if rank > registers[idx]:
            registers[idx] = rank
    z = 0.0
    zeros = 0
    for r in registers:
        z += 2.0**-r
        zeros += r == 0
    m = float(HLL_M)
    estimate = 0.7213 / (1.0 + 1.079 / m) * m * m / z
    if estimate <= 2.5 * m and zeros:
        estimate = m * math.log(m / zeros)
    return f"{n}:{math.floor(estimate + 0.5)}:{h31(registers)}"


# skiplist: towers of random height turn a linked list into a search tree

SKIP_MAX_LEVEL = 24
SKIP_LEVEL_SEED = 88675123


class SkipNode:
    __slots__ = ("key", "next")

    def __init__(self, key: int, height: int) -> None:
        self.key = key
        self.next: list[SkipNode | None] = [None] * height


def skiplist_input(n: int) -> tuple[list[int], list[int]]:
    keys = XorShift32().ints(n)
    return keys, [k ^ (i & 1) for i, k in enumerate(keys)]


def skiplist(keys: list[int], lookups: list[int]) -> tuple[SkipNode, int, int, int]:
    rng = XorShift32(SKIP_LEVEL_SEED)
    head = SkipNode(-1, SKIP_MAX_LEVEL)
    level = 1
    update: list[SkipNode] = [head] * SKIP_MAX_LEVEL
    heights = 0
    for key in keys:
        x = head
        for i in range(level - 1, -1, -1):
            nxt = x.next[i]
            while nxt is not None and nxt.key < key:
                x = nxt
                nxt = x.next[i]
            update[i] = x
        nxt = x.next[0]
        if nxt is not None and nxt.key == key:
            continue
        height = 1
        while height < SKIP_MAX_LEVEL and rng.next() & 1:
            height += 1
        for i in range(level, height):
            update[i] = head
        level = max(level, height)
        node = SkipNode(key, height)
        for i in range(height):
            node.next[i] = update[i].next[i]
            update[i].next[i] = node
        heights += height
    hits = 0
    for key in lookups:
        x = head
        for i in range(level - 1, -1, -1):
            nxt = x.next[i]
            while nxt is not None and nxt.key < key:
                x = nxt
                nxt = x.next[i]
        nxt = x.next[0]
        if nxt is not None and nxt.key == key:
            hits += 1
    return head, len(keys), hits, heights


def skiplist_checksum(result: tuple[SkipNode, int, int, int]) -> str:
    head, n, hits, heights = result
    ordered = []
    x = head.next[0]
    while x is not None:
        ordered.append(x.key)
        x = x.next[0]
    return f"{n}:{hits}:{heights}:{h31(ordered)}"


# cycle: Floyd's tortoise and hare on x -> x*x + c mod M

CYCLE_MOD = 67108859
CYCLE_C = 12345


def cycle_starts(n: int) -> list[int]:
    rng = XorShift32()
    return [rng.next() % CYCLE_MOD for _ in range(n)]


def cycle(starts: list[int]) -> str:
    m, c = CYCLE_MOD, CYCLE_C
    total_mu = total_lam = longest = 0
    for x0 in starts:
        tortoise = (x0 * x0 + c) % m
        hare = (tortoise * tortoise + c) % m
        while tortoise != hare:
            tortoise = (tortoise * tortoise + c) % m
            hare = (hare * hare + c) % m
            hare = (hare * hare + c) % m
        mu = 0
        tortoise = x0
        while tortoise != hare:
            tortoise = (tortoise * tortoise + c) % m
            hare = (hare * hare + c) % m
            mu += 1
        lam = 1
        hare = (tortoise * tortoise + c) % m
        while tortoise != hare:
            hare = (hare * hare + c) % m
            lam += 1
        total_mu += mu
        total_lam += lam
        longest = max(longest, mu + lam)
    return f"{len(starts)}:{total_mu}:{total_lam}:{longest}"


# palindrome: Manacher's mirror trick finds every palindrome in linear time


def binary_text(n: int) -> bytes:
    """`a`/`b` from the bits of generator values, low bit first, 32 characters per value."""
    rng = XorShift32()
    out = bytearray(n)
    for i in range(0, n, 32):
        v = rng.next()
        for j in range(i, min(i + 32, n)):
            out[j] = 98 if v & 1 else 97
            v >>= 1
    return bytes(out)


def palindrome(text: bytes) -> str:
    t = bytearray(b"#") * (2 * len(text) + 3)
    t[0], t[-1] = ord("^"), ord("$")
    t[2:-2:2] = text
    p = [0] * len(t)
    center = right = 0
    for i in range(1, len(t) - 1):
        if i < right:
            p[i] = min(right - i, p[2 * center - i])
        while t[i + 1 + p[i]] == t[i - 1 - p[i]]:
            p[i] += 1
        if i + p[i] > right:
            center, right = i, i + p[i]
    longest = best = count = 0
    for i, radius in enumerate(p):
        count += (radius + 1) // 2
        if radius > longest:
            longest, best = radius, i
    return f"{len(text)}:{longest}:{(best - 1 - longest) // 2}:{count}"


# bwt: suffix array by prefix doubling, then the Burrows-Wheeler transform of a DNA string

DNA = b"ACGT"


def dna_text(n: int) -> bytes:
    """`ACGT` from 2-bit chunks of generator values, low bits first, 16 characters per value."""
    rng = XorShift32()
    out = bytearray(n)
    for i in range(0, n, 16):
        v = rng.next()
        for j in range(i, min(i + 16, n)):
            out[j] = DNA[v & 3]
            v >>= 2
    return bytes(out)


def bwt(dna: bytes) -> tuple[int, int, int, bytes]:
    text = dna + b"$"
    n = len(text)
    rank = list(text)
    sa = list(range(n))
    k = 1
    rounds = 0
    while True:
        rounds += 1
        second = [rank[i + k] if i + k < n else -1 for i in range(n)]
        sa.sort(key=lambda i: (rank[i], second[i]))
        tmp = [0] * n
        for j in range(1, n):
            a, b = sa[j - 1], sa[j]
            tmp[b] = tmp[a] + (rank[a] != rank[b] or second[a] != second[b])
        rank = tmp
        if rank[sa[-1]] == n - 1:
            break
        k *= 2
    out = bytearray(n)
    primary = 0
    for j, i in enumerate(sa):
        if i == 0:
            primary = j
            out[j] = text[-1]
        else:
            out[j] = text[i - 1]
    return n, rounds, primary, bytes(out)


def bwt_checksum(result: tuple[int, int, int, bytes]) -> str:
    n, rounds, primary, out = result
    return f"{n}:{rounds}:{primary}:{fnv1a(out)}"


# aho-corasick: a trie with failure links matches thousands of patterns in one pass

AHO_PATTERNS = 10000


def aho_input(n: int) -> tuple[list[bytes], bytes]:
    rng = XorShift32()
    patterns = []
    for _ in range(AHO_PATTERNS):
        length = 6 + rng.next() % 7
        patterns.append(bytes(DNA[rng.next() % 4] for _ in range(length)))
    block = bytearray(ONE_MIB)
    for i in range(0, ONE_MIB, 16):
        v = rng.next()
        for j in range(i, i + 16):
            block[j] = DNA[v & 3]
            v >>= 2
    return patterns, bytes(block) * (n // ONE_MIB)


def aho_corasick(data: tuple[list[bytes], bytes]) -> str:
    patterns, text = data
    code = [0] * 256
    for i, ch in enumerate(DNA):
        code[ch] = i
    goto = [-1] * 4
    out = [0]
    for pattern in patterns:
        s = 0
        for ch in pattern:
            edge = s * 4 + code[ch]
            if goto[edge] == -1:
                goto[edge] = len(out)
                goto.extend((-1, -1, -1, -1))
                out.append(0)
            s = goto[edge]
        out[s] += 1
    fail = [0] * len(out)
    queue = []
    for c in range(4):
        if goto[c] == -1:
            goto[c] = 0
        else:
            queue.append(goto[c])
    head = 0
    while head < len(queue):
        s = queue[head]
        head += 1
        out[s] += out[fail[s]]
        for c in range(4):
            t = goto[s * 4 + c]
            if t == -1:
                goto[s * 4 + c] = goto[fail[s] * 4 + c]
            else:
                fail[t] = goto[fail[s] * 4 + c]
                queue.append(t)
    s = matches = hits = 0
    for ch in text:
        s = goto[s * 4 + code[ch]]
        found = out[s]
        if found:
            matches += found
            hits += 1
    return f"{len(patterns)}:{len(out)}:{matches}:{hits}"


# pi-digits: the Chudnovsky series, summed exactly by binary splitting

CHUD_A = 13591409
CHUD_B = 545140134
CHUD_C3_24 = 640320**3 // 24
DIGITS_PER_TERM = 14
GUARD_DIGITS = 10


def binary_split(a: int, b: int) -> tuple[int, int, int]:
    """P(a,b), Q(a,b), T(a,b) of the Chudnovsky terms a .. b-1."""
    if b - a == 1:
        if a == 0:
            p = q = 1
        else:
            p = (6 * a - 5) * (2 * a - 1) * (6 * a - 1)
            q = a * a * a * CHUD_C3_24
        t = p * (CHUD_A + CHUD_B * a)
        return p, q, -t if a % 2 else t
    m = (a + b) // 2
    p1, q1, t1 = binary_split(a, m)
    p2, q2, t2 = binary_split(m, b)
    return p1 * p2, q1 * q2, t1 * q2 + p1 * t2


def isqrt(n: int) -> int:
    """Floor square root of n > 0 by precision doubling (math.isqrt's algorithm, written out as in the others)."""
    c = (n.bit_length() - 1) // 2
    a = 1
    d = 0
    for s in reversed(range(c.bit_length())):
        e = d
        d = c >> s
        a = (a << (d - e - 1)) + (n >> (2 * c - e - d + 1)) // a
    return a - 1 if a * a > n else a


def pi_digits(n: int) -> str:
    terms = n // DIGITS_PER_TERM + 2
    _p, q, t = binary_split(0, terms)
    one = 10 ** (n + GUARD_DIGITS)
    sqrt_c = isqrt(10005 * one * one)
    pi = 426880 * sqrt_c * q // t
    sys.set_int_max_str_digits(0)
    return str(pi // 10**GUARD_DIGITS)


def pi_checksum(digits: str) -> str:
    return f"{len(digits) - 1}:{digits[-50:]}:{sum(map(int, digits))}"


# life: Conway's Game of Life on a torus

LIFE_SIDE = 512


def life_input(gens: int) -> tuple[bytes, int]:
    rng = XorShift32()
    return bytes(1 if rng.next() % 4 == 0 else 0 for _ in range(LIFE_SIDE * LIFE_SIDE)), gens


def life(grid: bytes, gens: int) -> tuple[int, bytearray]:
    w = LIFE_SIDE
    cur = bytearray(grid)
    nxt = bytearray(len(grid))
    for _ in range(gens):
        for y in range(w):
            up = (y + w - 1) % w * w
            row = y * w
            down = (y + 1) % w * w
            for x in range(w):
                left = (x + w - 1) % w
                right = (x + 1) % w
                n = (
                    cur[up + left]
                    + cur[up + x]
                    + cur[up + right]
                    + cur[row + left]
                    + cur[row + right]
                    + cur[down + left]
                    + cur[down + x]
                    + cur[down + right]
                )
                nxt[row + x] = 1 if n == 3 or (n == 2 and cur[row + x]) else 0
        cur, nxt = nxt, cur
    return gens, cur


def life_checksum(result: tuple[int, bytearray]) -> str:
    gens, grid = result
    return f"{gens}:{sum(grid)}:{fnv1a(grid)}"


# huffman: merge the two rarest symbols until one tree is left


def skewed_text(n: int) -> bytes:
    """A 1 MiB block of letters skewed toward `a` (two generator values per letter), repeated."""
    rng = XorShift32()
    block = bytes(97 + (rng.next() % 26) * (rng.next() % 26) // 26 for _ in range(ONE_MIB))
    return block * (n // ONE_MIB)


def huffman(text: bytes) -> tuple[int, int, bytes, bool]:
    freq = [0] * 256
    for b in text:
        freq[b] += 1
    heap = [f * 1024 + sym for sym, f in enumerate(freq) if f]
    heapq.heapify(heap)
    left = [0] * 512
    right = [0] * 512
    next_id = 256
    while len(heap) > 1:
        a = heapq.heappop(heap)
        b = heapq.heappop(heap)
        left[next_id], right[next_id] = a % 1024, b % 1024
        heapq.heappush(heap, (a // 1024 + b // 1024) * 1024 + next_id)
        next_id += 1
    root = heap[0] % 1024
    codes = [0] * 256
    lengths = [0] * 256
    stack = [(root, 0, 0)]
    while stack:
        node, code, length = stack.pop()
        if node < 256:
            codes[node], lengths[node] = code, length
        else:
            stack.append((left[node], code << 1, length + 1))
            stack.append((right[node], (code << 1) | 1, length + 1))
    encoded = bytearray()
    acc = nbits = bits = 0
    for b in text:
        acc = (acc << lengths[b]) | codes[b]
        nbits += lengths[b]
        bits += lengths[b]
        while nbits >= 8:
            nbits -= 8
            encoded.append((acc >> nbits) & 0xFF)
        acc &= (1 << nbits) - 1
    if nbits:
        encoded.append((acc << (8 - nbits)) & 0xFF)
    decoded = bytearray(len(text))
    node = root
    k = 0
    for byte in encoded:
        for shift in range(7, -1, -1):
            node = right[node] if (byte >> shift) & 1 else left[node]
            if node < 256:
                decoded[k] = node
                k += 1
                node = root
                if k == len(text):
                    break
    return len(text), bits, bytes(encoded), decoded == text


def huffman_checksum(result: tuple[int, int, bytes, bool]) -> str:
    n, bits, encoded, ok = result
    return f"{n}:{bits}:{fnv1a(encoded)}:{int(ok)}"


# window-max: a monotonic deque keeps the maximum of every window in O(1) per step

WINDOWS = (10, 100, 1000, 10000, 100000)


def window_max(values: list[int]) -> str:
    n = len(values)
    dq = [0] * n
    hashes = []
    for w in WINDOWS:
        head = tail = 0
        h = 0
        for i in range(n):
            v = values[i]
            while tail > head and values[dq[tail - 1]] <= v:
                tail -= 1
            dq[tail] = i
            tail += 1
            if dq[head] <= i - w:
                head += 1
            if i >= w - 1:
                h = (h * 31 + values[dq[head]]) & MASK
        hashes.append(str(h))
    return f"{n}:" + ":".join(hashes)


# rain-water: flood a 2D terrain from its walls inward with a min-heap


def terrain(side: int) -> list[int]:
    rng = XorShift32()
    return [rng.next() % 1000 for _ in range(side * side)]


def rain_water(heights: list[int]) -> str:
    n = len(heights)
    side = math.isqrt(n)
    seen = bytearray(n)
    heap = []
    for idx in range(n):
        r, c = divmod(idx, side)
        if r in (0, side - 1) or c in (0, side - 1):
            seen[idx] = 1
            heap.append(heights[idx] * n + idx)
    heapq.heapify(heap)
    water = wet = 0
    while heap:
        level, idx = divmod(heapq.heappop(heap), n)
        r, c = divmod(idx, side)
        for nr, nc in ((r - 1, c), (r + 1, c), (r, c - 1), (r, c + 1)):
            if 0 <= nr < side and 0 <= nc < side:
                nidx = nr * side + nc
                if not seen[nidx]:
                    seen[nidx] = 1
                    h = heights[nidx]
                    if h < level:
                        water += level - h
                        wet += 1
                        h = level
                    heapq.heappush(heap, h * n + nidx)
    return f"{side}:{water}:{wet}"


# collatz: remember every chain length below the start so each walk stops early


def collatz(n: int) -> str:
    cache = [0] * n
    cache[1] = 1
    best, best_len, total = 1, 1, 1
    for start in range(2, n):
        x = start
        steps = 0
        while x >= start:
            x = x // 2 if x % 2 == 0 else 3 * x + 1
            steps += 1
        length = steps + cache[x]
        cache[start] = length
        total += length
        if length > best_len:
            best, best_len = start, length
    return f"{n}:{best}:{best_len}:{total}"


def kernels() -> dict:
    return {
        "sudoku": {"prepare": lambda n, _d: sudoku_puzzles(n), "run": lambda p, _t, _p: sudoku(p), "check": str},
        "bloom": {"prepare": lambda n, _d: bloom_input(n), "run": lambda kv, _t, _p: bloom(*kv), "check": str},
        "hyperloglog": {"run": lambda n, _t, _p: hyperloglog(n), "check": str},
        "skiplist": {
            "prepare": lambda n, _d: skiplist_input(n),
            "run": lambda kv, _t, _p: skiplist(*kv),
            "check": skiplist_checksum,
        },
        "cycle": {"prepare": lambda n, _d: cycle_starts(n), "run": lambda s, _t, _p: cycle(s), "check": str},
        "palindrome": {
            "prepare": lambda n, _d: binary_text(n),
            "run": lambda s, _t, _p: palindrome(s),
            "check": str,
        },
        "bwt": {"prepare": lambda n, _d: dna_text(n), "run": lambda s, _t, _p: bwt(s), "check": bwt_checksum},
        "aho-corasick": {
            "prepare": lambda n, _d: aho_input(n),
            "run": lambda d, _t, _p: aho_corasick(d),
            "check": str,
        },
        "pi-digits": {"run": lambda n, _t, _p: pi_digits(n), "check": pi_checksum},
        "life": {"prepare": lambda n, _d: life_input(n), "run": lambda g, _t, _p: life(*g), "check": life_checksum},
        "huffman": {
            "prepare": lambda n, _d: skewed_text(n),
            "run": lambda s, _t, _p: huffman(s),
            "check": huffman_checksum,
        },
        "window-max": {
            "prepare": lambda n, _d: XorShift32().ints(n),
            "run": lambda v, _t, _p: window_max(v),
            "check": str,
        },
        "rain-water": {"prepare": lambda n, _d: terrain(n), "run": lambda h, _t, _p: rain_water(h), "check": str},
        "collatz": {"run": lambda n, _t, _p: collatz(n), "check": str},
    }
