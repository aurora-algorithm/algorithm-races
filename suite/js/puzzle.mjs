// Unusual algorithm kernels (kernels/SPEC.md section 13): one clever trick each, the same steps in every language.

import { fnv1a, h31, XorShift32 } from "./gen.mjs";
import {
	ahoCorasick,
	ahoInput,
	binaryText,
	bwt,
	bwtChecksum,
	dnaText,
	huffman,
	huffmanChecksum,
	MinHeap,
	palindrome,
	skewedText,
} from "./puzzle-text.mjs";

// MurmurHash3's 32-bit finalizer: the one hash every language uses for the probabilistic kernels.
function fmix32(key) {
	let h = key;
	h ^= h >>> 16;
	h = Math.imul(h, 0x85ebca6b);
	h ^= h >>> 13;
	h = Math.imul(h, 0xc2b2ae35);
	h ^= h >>> 16;
	return h >>> 0;
}

function shuffle(rng, items) {
	for (let i = items.length - 1; i > 0; i--) {
		const j = rng.next() % (i + 1);
		[items[i], items[j]] = [items[j], items[i]];
	}
}

const range = (n) => Array.from({ length: n }, (_, i) => i);

// sudoku: Knuth's Algorithm X with dancing links

const SUDOKU_CLUES = 26;
const DLX_COLUMNS = 324;

function sudokuPuzzles(n) {
	const rng = new XorShift32();
	const puzzles = [];
	for (let p = 0; p < n; p++) {
		const digits = range(9).map((i) => i + 1);
		shuffle(rng, digits);
		const order = [];
		for (let axis = 0; axis < 2; axis++) {
			const bands = [0, 1, 2];
			shuffle(rng, bands);
			const lines = [];
			for (const band of bands) {
				const inner = [0, 1, 2];
				shuffle(rng, inner);
				for (const i of inner) lines.push(band * 3 + i);
			}
			order.push(lines);
		}
		const [rows, cols] = order;
		const cells = range(81);
		shuffle(rng, cells);
		const puzzle = new Array(81).fill(0);
		for (const cell of cells.slice(0, SUDOKU_CLUES)) {
			const r = rows[Math.floor(cell / 9)];
			const c = cols[cell % 9];
			puzzle[cell] = digits[(r * 3 + Math.floor(r / 3) + c) % 9];
		}
		puzzles.push(puzzle);
	}
	return puzzles;
}

// The exact-cover matrix as flat arrays: node 0 is the root, 1..324 the column headers, then 4 nodes per row.
class Dlx {
	constructor() {
		const size = 1 + DLX_COLUMNS + 729 * 4;
		this.left = new Int32Array(size);
		this.right = new Int32Array(size);
		this.up = Int32Array.from(range(size));
		this.down = Int32Array.from(range(size));
		this.col = Int32Array.from(range(size));
		this.row = new Int32Array(size);
		this.count = new Int32Array(DLX_COLUMNS + 1);
		for (let i = 0; i <= DLX_COLUMNS; i++) {
			this.left[i] = i - 1;
			this.right[i] = i + 1;
		}
		this.left[0] = DLX_COLUMNS;
		this.right[DLX_COLUMNS] = 0;
		let node = DLX_COLUMNS + 1;
		for (let r = 0; r < 9; r++) {
			for (let c = 0; c < 9; c++) {
				const box = Math.floor(r / 3) * 3 + Math.floor(c / 3);
				for (let d = 0; d < 9; d++) {
					const first = node;
					const columns = [
						1 + r * 9 + c,
						82 + r * 9 + d,
						163 + c * 9 + d,
						244 + box * 9 + d,
					];
					for (const column of columns) {
						this.col[node] = column;
						this.row[node] = (r * 9 + c) * 9 + d;
						this.up[node] = this.up[column];
						this.down[node] = column;
						this.down[this.up[column]] = node;
						this.up[column] = node;
						this.count[column]++;
						this.left[node] = node - 1;
						this.right[node] = node + 1;
						node++;
					}
					this.left[first] = node - 1;
					this.right[node - 1] = first;
				}
			}
		}
		this.solutions = 0;
		this.tries = 0;
		this.chosen = new Int32Array(81);
		this.first = [];
	}

	cover(c) {
		const { left, right, up, down, col, count } = this;
		right[left[c]] = right[c];
		left[right[c]] = left[c];
		for (let i = down[c]; i !== c; i = down[i]) {
			for (let j = right[i]; j !== i; j = right[j]) {
				down[up[j]] = down[j];
				up[down[j]] = up[j];
				count[col[j]]--;
			}
		}
	}

	uncover(c) {
		const { left, right, up, down, col, count } = this;
		for (let i = up[c]; i !== c; i = up[i]) {
			for (let j = left[i]; j !== i; j = left[j]) {
				count[col[j]]++;
				down[up[j]] = j;
				up[down[j]] = j;
			}
		}
		right[left[c]] = c;
		left[right[c]] = c;
	}

	chooseRow(node) {
		for (let j = this.right[node]; j !== node; j = this.right[j]) {
			this.cover(this.col[j]);
		}
	}

	unchooseRow(node) {
		for (let j = this.left[node]; j !== node; j = this.left[j]) {
			this.uncover(this.col[j]);
		}
	}

	search(depth) {
		const { right, count } = this;
		if (right[0] === 0) {
			this.solutions++;
			if (this.solutions === 1)
				this.first = Array.from(this.chosen.subarray(0, depth));
			return;
		}
		let best = right[0];
		for (let c = right[best]; c !== 0; c = right[c]) {
			if (count[c] < count[best]) best = c;
		}
		this.cover(best);
		for (let r = this.down[best]; r !== best; r = this.down[r]) {
			this.tries++;
			this.chosen[depth] = this.row[r];
			this.chooseRow(r);
			this.search(depth + 1);
			this.unchooseRow(r);
		}
		this.uncover(best);
	}
}

function sudoku(puzzles) {
	let solutions = 0;
	let tries = 0;
	let h = 0;
	for (const puzzle of puzzles) {
		const dlx = new Dlx();
		let depth = 0;
		puzzle.forEach((digit, cell) => {
			if (digit === 0) return;
			const node = DLX_COLUMNS + 1 + (cell * 9 + digit - 1) * 4;
			dlx.cover(dlx.col[node]);
			dlx.chooseRow(node);
			dlx.chosen[depth] = cell * 9 + digit - 1;
			depth++;
		});
		dlx.search(depth);
		solutions += dlx.solutions;
		tries += dlx.tries;
		const grid = new Array(81).fill(0);
		for (const row of dlx.first) grid[Math.floor(row / 9)] = (row % 9) + 1;
		h = h31([h, ...grid]);
	}
	return `${puzzles.length}:${solutions}:${tries}:${h}`;
}

// bloom: k bit positions per key from two hashes

const BLOOM_BITS_PER_KEY = 10;
const BLOOM_HASHES = 7;
const GOLDEN = 0x9e3779b9;

function bloomInput(n) {
	const values = new XorShift32().ints(2 * n);
	return { keys: values.slice(0, n), lookups: values.slice(n) };
}

// SWAR popcount of one byte, as the other languages' popcount instructions count it.
function popcount8(byte) {
	const b2 = byte - ((byte >> 1) & 0x55);
	const b4 = (b2 & 0x33) + ((b2 >> 2) & 0x33);
	return (b4 + (b4 >> 4)) & 0x0f;
}

function bloom({ keys, lookups }) {
	const m = BLOOM_BITS_PER_KEY * keys.length;
	const bits = new Uint8Array(Math.ceil(m / 8));
	for (const key of keys) {
		const h1 = fmix32(key);
		const h2 = (fmix32((key ^ GOLDEN) >>> 0) | 1) >>> 0;
		for (let i = 0; i < BLOOM_HASHES; i++) {
			const b = ((h1 + i * h2) >>> 0) % m;
			bits[b >> 3] |= 1 << (b & 7);
		}
	}
	let falsePositives = 0;
	for (const key of lookups) {
		const h1 = fmix32(key);
		const h2 = (fmix32((key ^ GOLDEN) >>> 0) | 1) >>> 0;
		let all = true;
		for (let i = 0; i < BLOOM_HASHES; i++) {
			const b = ((h1 + i * h2) >>> 0) % m;
			if ((bits[b >> 3] & (1 << (b & 7))) === 0) {
				all = false;
				break;
			}
		}
		if (all) falsePositives++;
	}
	let setBits = 0;
	for (const byte of bits) setBits += popcount8(byte);
	return `${keys.length}:${setBits}:${falsePositives}`;
}

// hyperloglog: the longest run of leading zeros estimates how many distinct items went by

const HLL_P = 14;
const HLL_M = 1 << HLL_P;

function hyperloglog(n) {
	const rng = new XorShift32();
	const universe = Math.max(Math.floor(n / 4), 1);
	const registers = new Uint8Array(HLL_M);
	for (let i = 0; i < n; i++) {
		const h = fmix32(rng.next() % universe);
		const w = (h << HLL_P) >>> 0;
		const rank = w !== 0 ? Math.clz32(w) + 1 : 32 - HLL_P + 1;
		const idx = h >>> (32 - HLL_P);
		if (rank > registers[idx]) registers[idx] = rank;
	}
	return hllResult(n, registers);
}

// The same loop without `>>> 0`: clz32 reads the signed int32 bits the same way (runtime node-hllclz, deno-hllclz).
function hyperloglogClz(n) {
	const rng = new XorShift32();
	const universe = Math.max(Math.floor(n / 4), 1);
	const registers = new Uint8Array(HLL_M);
	for (let i = 0; i < n; i++) {
		const h = fmix32(rng.next() % universe);
		const w = h << HLL_P;
		const rank = w !== 0 ? Math.clz32(w) + 1 : 32 - HLL_P + 1;
		const idx = h >>> (32 - HLL_P);
		if (rank > registers[idx]) registers[idx] = rank;
	}
	return hllResult(n, registers);
}

function hllResult(n, registers) {
	let z = 0;
	let zeros = 0;
	for (const r of registers) {
		z += 2 ** -r;
		if (r === 0) zeros++;
	}
	const m = HLL_M;
	let estimate = ((0.7213 / (1 + 1.079 / m)) * m * m) / z;
	if (estimate <= 2.5 * m && zeros > 0) estimate = m * Math.log(m / zeros);
	return `${n}:${Math.floor(estimate + 0.5)}:${h31(registers)}`;
}

// skiplist: towers of random height turn a linked list into a search tree

const SKIP_MAX_LEVEL = 24;
const SKIP_LEVEL_SEED = 88675123;

class SkipNode {
	constructor(key, height) {
		this.key = key;
		this.next = new Array(height).fill(null);
	}
}

function skiplistInput(n) {
	const keys = new XorShift32().ints(n);
	return { keys, lookups: keys.map((k, i) => (k ^ (i & 1)) >>> 0) };
}

function skiplist({ keys, lookups }) {
	const rng = new XorShift32(SKIP_LEVEL_SEED);
	const head = new SkipNode(-1, SKIP_MAX_LEVEL);
	let level = 1;
	const update = new Array(SKIP_MAX_LEVEL).fill(head);
	let heights = 0;
	for (const key of keys) {
		let x = head;
		for (let i = level - 1; i >= 0; i--) {
			while (x.next[i] !== null && x.next[i].key < key) x = x.next[i];
			update[i] = x;
		}
		const found = x.next[0];
		if (found !== null && found.key === key) continue;
		let height = 1;
		while (height < SKIP_MAX_LEVEL && (rng.next() & 1) === 1) height++;
		for (let i = level; i < height; i++) update[i] = head;
		level = Math.max(level, height);
		const node = new SkipNode(key, height);
		for (let i = 0; i < height; i++) {
			node.next[i] = update[i].next[i];
			update[i].next[i] = node;
		}
		heights += height;
	}
	let hits = 0;
	for (const key of lookups) {
		let x = head;
		for (let i = level - 1; i >= 0; i--) {
			while (x.next[i] !== null && x.next[i].key < key) x = x.next[i];
		}
		const found = x.next[0];
		if (found !== null && found.key === key) hits++;
	}
	return { head, n: keys.length, hits, heights };
}

function skiplistChecksum({ head, n, hits, heights }) {
	const ordered = [];
	for (let x = head.next[0]; x !== null; x = x.next[0]) ordered.push(x.key);
	return `${n}:${hits}:${heights}:${h31(ordered)}`;
}

// cycle: Floyd's tortoise and hare on x -> x*x + c mod M

const CYCLE_MOD = 67108859;
const CYCLE_C = 12345;

function cycleStarts(n) {
	const rng = new XorShift32();
	return Array.from({ length: n }, () => rng.next() % CYCLE_MOD);
}

const step = (x) => (x * x + CYCLE_C) % CYCLE_MOD;

function cycle(starts) {
	let totalMu = 0;
	let totalLam = 0;
	let longest = 0;
	for (const x0 of starts) {
		let tortoise = step(x0);
		let hare = step(tortoise);
		while (tortoise !== hare) {
			tortoise = step(tortoise);
			hare = step(step(hare));
		}
		let mu = 0;
		tortoise = x0;
		while (tortoise !== hare) {
			tortoise = step(tortoise);
			hare = step(hare);
			mu++;
		}
		let lam = 1;
		hare = step(tortoise);
		while (tortoise !== hare) {
			hare = step(hare);
			lam++;
		}
		totalMu += mu;
		totalLam += lam;
		longest = Math.max(longest, mu + lam);
	}
	return `${starts.length}:${totalMu}:${totalLam}:${longest}`;
}

// pi-digits: the Chudnovsky series, summed exactly by binary splitting

const CHUD_A = 13591409n;
const CHUD_B = 545140134n;
const CHUD_C3_24 = 640320n ** 3n / 24n;
const DIGITS_PER_TERM = 14;
const GUARD_DIGITS = 10n;

// P(a,b), Q(a,b), T(a,b) of the Chudnovsky terms a .. b-1.
function binarySplit(a, b) {
	if (b - a === 1) {
		const k = BigInt(a);
		const p = a === 0 ? 1n : (6n * k - 5n) * (2n * k - 1n) * (6n * k - 1n);
		const q = a === 0 ? 1n : k * k * k * CHUD_C3_24;
		const t = p * (CHUD_A + CHUD_B * k);
		return [p, q, a % 2 === 1 ? -t : t];
	}
	const m = Math.floor((a + b) / 2);
	const [p1, q1, t1] = binarySplit(a, m);
	const [p2, q2, t2] = binarySplit(m, b);
	return [p1 * p2, q1 * q2, t1 * q2 + p1 * t2];
}

// Floor square root of n > 0 by precision doubling (CPython's math.isqrt): each step doubles the correct bits.
function isqrt(n) {
	const c = Math.floor((n.toString(2).length - 1) / 2);
	let a = 1n;
	let d = 0;
	for (let s = 31 - Math.clz32(c); s >= 0; s--) {
		const e = d;
		d = c >>> s;
		a = (a << BigInt(d - e - 1)) + (n >> BigInt(2 * c - e - d + 1)) / a;
	}
	return a * a > n ? a - 1n : a;
}

function piDigits(n) {
	const terms = Math.floor(n / DIGITS_PER_TERM) + 2;
	const [, q, t] = binarySplit(0, terms);
	const one = 10n ** (BigInt(n) + GUARD_DIGITS);
	const sqrtC = isqrt(10005n * one * one);
	const pi = (426880n * sqrtC * q) / t;
	return (pi / 10n ** GUARD_DIGITS).toString();
}

function piChecksum(digits) {
	let sum = 0;
	for (const ch of digits) sum += Number(ch);
	return `${digits.length - 1}:${digits.slice(-50)}:${sum}`;
}

// life: Conway's Game of Life on a torus

const LIFE_SIDE = 512;

function lifeInput(gens) {
	const rng = new XorShift32();
	const grid = Uint8Array.from({ length: LIFE_SIDE * LIFE_SIDE }, () =>
		rng.next() % 4 === 0 ? 1 : 0,
	);
	return { grid, gens };
}

function life({ grid, gens }) {
	const w = LIFE_SIDE;
	let cur = grid.slice();
	let next = new Uint8Array(grid.length);
	for (let g = 0; g < gens; g++) {
		for (let y = 0; y < w; y++) {
			const up = ((y + w - 1) % w) * w;
			const row = y * w;
			const down = ((y + 1) % w) * w;
			for (let x = 0; x < w; x++) {
				const left = (x + w - 1) % w;
				const right = (x + 1) % w;
				const n =
					cur[up + left] +
					cur[up + x] +
					cur[up + right] +
					cur[row + left] +
					cur[row + right] +
					cur[down + left] +
					cur[down + x] +
					cur[down + right];
				next[row + x] = n === 3 || (n === 2 && cur[row + x] === 1) ? 1 : 0;
			}
		}
		[cur, next] = [next, cur];
	}
	return { gens, grid: cur };
}

function lifeChecksum({ gens, grid }) {
	let alive = 0;
	for (const cell of grid) alive += cell;
	return `${gens}:${alive}:${fnv1a(grid)}`;
}

// window-max: a monotonic deque keeps the maximum of every window in O(1) per step

const WINDOWS = [10, 100, 1000, 10000, 100000];

function windowMax(values) {
	const n = values.length;
	const dq = new Int32Array(n);
	const hashes = [];
	for (const w of WINDOWS) {
		let head = 0;
		let tail = 0;
		let h = 0;
		for (let i = 0; i < n; i++) {
			const v = values[i];
			while (tail > head && values[dq[tail - 1]] <= v) tail--;
			dq[tail++] = i;
			if (dq[head] <= i - w) head++;
			if (i >= w - 1) h = (Math.imul(h, 31) + values[dq[head]]) >>> 0;
		}
		hashes.push(h);
	}
	return `${n}:${hashes.join(":")}`;
}

// rain-water: flood a 2D terrain from its walls inward with a min-heap

function terrain(side) {
	const rng = new XorShift32();
	return Array.from({ length: side * side }, () => rng.next() % 1000);
}

const RAIN_DR = [-1, 1, 0, 0];
const RAIN_DC = [0, 0, -1, 1];

function rainWater(heights) {
	const n = heights.length;
	const side = Math.round(Math.sqrt(n));
	const seen = new Uint8Array(n);
	const heap = new MinHeap();
	for (let idx = 0; idx < n; idx++) {
		const r = Math.floor(idx / side);
		const c = idx % side;
		if (r === 0 || r === side - 1 || c === 0 || c === side - 1) {
			seen[idx] = 1;
			heap.push(heights[idx] * n + idx);
		}
	}
	let water = 0;
	let wet = 0;
	while (heap.size > 0) {
		const key = heap.pop();
		const level = Math.floor(key / n);
		const idx = key % n;
		const r = Math.floor(idx / side);
		const c = idx % side;
		for (let d = 0; d < 4; d++) {
			const nr = r + RAIN_DR[d];
			const nc = c + RAIN_DC[d];
			if (nr < 0 || nr >= side || nc < 0 || nc >= side) continue;
			const nidx = nr * side + nc;
			if (seen[nidx] === 1) continue;
			seen[nidx] = 1;
			let h = heights[nidx];
			if (h < level) {
				water += level - h;
				wet++;
				h = level;
			}
			heap.push(h * n + nidx);
		}
	}
	return `${side}:${water}:${wet}`;
}

// collatz: remember every chain length below the start so each walk stops early

function collatz(n) {
	const cache = new Int32Array(n);
	cache[1] = 1;
	let best = 1;
	let bestLength = 1;
	let total = 1;
	for (let start = 2; start < n; start++) {
		let x = start;
		let steps = 0;
		while (x >= start) {
			x = x % 2 === 0 ? x / 2 : 3 * x + 1;
			steps++;
		}
		const length = steps + cache[x];
		cache[start] = length;
		total += length;
		if (length > bestLength) {
			best = start;
			bestLength = length;
		}
	}
	return `${n}:${best}:${bestLength}:${total}`;
}

export const PUZZLE_KERNELS = {
	sudoku: { prepare: sudokuPuzzles, run: sudoku, check: String },
	bloom: { prepare: bloomInput, run: bloom, check: String },
	hyperloglog: {
		run:
			globalThis.process?.env?.KERNELS_HLL === "clz"
				? hyperloglogClz
				: hyperloglog,
		check: String,
	},
	skiplist: { prepare: skiplistInput, run: skiplist, check: skiplistChecksum },
	cycle: { prepare: cycleStarts, run: cycle, check: String },
	palindrome: { prepare: binaryText, run: palindrome, check: String },
	bwt: { prepare: dnaText, run: bwt, check: bwtChecksum },
	"aho-corasick": { prepare: ahoInput, run: ahoCorasick, check: String },
	"pi-digits": { run: piDigits, check: piChecksum },
	life: { prepare: lifeInput, run: life, check: lifeChecksum },
	huffman: { prepare: skewedText, run: huffman, check: huffmanChecksum },
	"window-max": {
		prepare: (n) => new XorShift32().ints(n),
		run: windowMax,
		check: String,
	},
	"rain-water": { prepare: terrain, run: rainWater, check: String },
	collatz: { run: collatz, check: String },
};
