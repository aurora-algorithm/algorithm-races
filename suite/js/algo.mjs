// Classic algorithm kernels (kernels/SPEC.md section 12, group algo).

import { h31, XorShift32 } from "./gen.mjs";

const KNAPSACK_ITEMS = 1000;
const PATTERN_LENGTH = 8;
const ACGT = [..."acgt"].map((c) => c.charCodeAt(0));

function grid(n) {
	const rng = new XorShift32();
	const walls = new Uint8Array(n * n);
	for (let i = 0; i < walls.length; i++) walls[i] = rng.next() % 100 < 30;
	for (const cell of [0, 1, n, n * n - n - 1, n * n - 2, n * n - 1]) {
		walls[cell] = 0;
	}
	return { n, walls };
}

function bfsGrid({ n, walls }) {
	const dist = new Int32Array(n * n).fill(-1);
	dist[0] = 0;
	const queue = [0];
	const visit = (next, ok, d) => {
		if (ok && !walls[next] && dist[next] < 0) {
			dist[next] = d;
			queue.push(next);
		}
	};
	for (let head = 0; head < queue.length; head++) {
		const cell = queue[head];
		const d = dist[cell] + 1;
		const row = Math.floor(cell / n);
		const col = cell % n;
		visit(cell - n, row > 0, d);
		visit(cell + n, row < n - 1, d);
		visit(cell - 1, col > 0, d);
		visit(cell + 1, col < n - 1, d);
	}
	let reached = 0;
	let total = 0;
	for (const d of dist) {
		if (d >= 0) {
			reached++;
			total += d;
		}
	}
	return `${reached}:${dist[n * n - 1]}:${total}`;
}

// Four edges per node u at 4u..4u+3: one to u+1 (a ring, so every node is reachable), three random.
function graph(n) {
	const rng = new XorShift32();
	const to = new Int32Array(4 * n);
	const weight = new Int32Array(4 * n);
	for (let u = 0; u < n; u++) {
		to[4 * u] = (u + 1) % n;
		weight[4 * u] = 1 + (rng.next() % 100);
		for (let e = 4 * u + 1; e < 4 * u + 4; e++) {
			to[e] = rng.next() % n;
			weight[e] = 1 + (rng.next() % 100);
		}
	}
	return { n, to, weight };
}

// JavaScript has no standard priority queue: a binary min-heap of [dist, node] pairs.
class MinHeap {
	items = [];

	get size() {
		return this.items.length;
	}

	push(item) {
		const a = this.items;
		a.push(item);
		let i = a.length - 1;
		while (i > 0) {
			const parent = (i - 1) >> 1;
			if (a[parent][0] <= item[0]) break;
			a[i] = a[parent];
			i = parent;
		}
		a[i] = item;
	}

	pop() {
		const a = this.items;
		const top = a[0];
		const last = a.pop();
		if (a.length === 0) return top;
		let i = 0;
		for (;;) {
			let child = 2 * i + 1;
			if (child >= a.length) break;
			if (child + 1 < a.length && a[child + 1][0] < a[child][0]) child++;
			if (a[child][0] >= last[0]) break;
			a[i] = a[child];
			i = child;
		}
		a[i] = last;
		return top;
	}
}

function dijkstra({ n, to, weight }) {
	const dist = new Array(n).fill(Number.POSITIVE_INFINITY);
	dist[0] = 0;
	const heap = new MinHeap();
	heap.push([0, 0]);
	while (heap.size > 0) {
		const [d, u] = heap.pop();
		if (d > dist[u]) continue;
		for (let e = 4 * u; e < 4 * u + 4; e++) {
			const v = to[e];
			const nd = d + weight[e];
			if (nd < dist[v]) {
				dist[v] = nd;
				heap.push([nd, v]);
			}
		}
	}
	let reached = 0;
	let total = 0;
	let longest = 0;
	for (const d of dist) {
		if (d < Number.POSITIVE_INFINITY) {
			reached++;
			total += d;
			longest = Math.max(longest, d);
		}
	}
	return `${reached}:${total}:${longest}`;
}

function edgeList(n) {
	const rng = new XorShift32();
	const edges = [];
	for (let i = 0; i < 4 * n; i++) {
		const u = rng.next() % n;
		const v = rng.next() % n;
		edges.push({ w: rng.next() % 1000000, u, v });
	}
	return { n, edges };
}

function find(parent, x) {
	let root = x;
	while (parent[root] !== root) root = parent[root];
	while (parent[x] !== root) {
		const next = parent[x];
		parent[x] = root;
		x = next;
	}
	return root;
}

function kruskal({ n, edges }) {
	edges.sort((a, b) => a.w - b.w);
	const parent = new Int32Array(n);
	for (let i = 0; i < n; i++) parent[i] = i;
	const rank = new Uint8Array(n);
	let count = 0;
	let total = 0;
	for (const e of edges) {
		let ru = find(parent, e.u);
		let rv = find(parent, e.v);
		if (ru === rv) continue;
		if (rank[ru] < rank[rv]) [ru, rv] = [rv, ru];
		parent[rv] = ru;
		if (rank[ru] === rank[rv]) rank[ru]++;
		count++;
		total += e.w;
	}
	return `${count}:${total}`;
}

export function hashmapInput(n) {
	const inserts = new XorShift32().ints(n);
	return { inserts, lookups: inserts.map((k, i) => (k ^ (i & 1)) >>> 0) };
}

class Node {
	constructor(key) {
		this.key = key;
		this.count = 1;
		this.left = null;
		this.right = null;
	}
}

function inOrder(root) {
	const keys = [];
	const stack = [];
	let node = root;
	while (node !== null || stack.length > 0) {
		while (node !== null) {
			stack.push(node);
			node = node.left;
		}
		node = stack.pop();
		keys.push(node.key);
		node = node.right;
	}
	return keys;
}

function bst({ inserts, lookups }) {
	let root = null;
	let size = 0;
	for (const k of inserts) {
		if (root === null) {
			root = new Node(k);
			size++;
			continue;
		}
		let node = root;
		for (;;) {
			if (k < node.key) {
				if (node.left === null) {
					node.left = new Node(k);
					size++;
					break;
				}
				node = node.left;
			} else if (k > node.key) {
				if (node.right === null) {
					node.right = new Node(k);
					size++;
					break;
				}
				node = node.right;
			} else {
				node.count++;
				break;
			}
		}
	}
	let hits = 0;
	for (const k of lookups) {
		let node = root;
		while (node !== null && node.key !== k) {
			node = k < node.key ? node.left : node.right;
		}
		if (node !== null) hits++;
	}
	return `${size}:${hits}:${h31(inOrder(root))}`;
}

function word(rng, minLength, spread) {
	const length = minLength + (rng.next() % spread);
	let s = "";
	for (let i = 0; i < length; i++) {
		s += String.fromCharCode(97 + (rng.next() % 26));
	}
	return s;
}

export function words(n) {
	const rng = new XorShift32();
	const inserted = [];
	for (let i = 0; i < n; i++) inserted.push(word(rng, 3, 8));
	const prefixes = [];
	for (let i = 0; i < n; i++) prefixes.push(word(rng, 1, 4));
	return { inserted, prefixes };
}

class TrieNode {
	count = 0;
	children = new Array(26).fill(null);
}

function trie({ inserted, prefixes }) {
	const root = new TrieNode();
	let nodes = 0;
	for (const w of inserted) {
		let node = root;
		for (let i = 0; i < w.length; i++) {
			const c = w.charCodeAt(i) - 97;
			let child = node.children[c];
			if (child === null) {
				child = new TrieNode();
				node.children[c] = child;
				nodes++;
			}
			child.count++;
			node = child;
		}
	}
	let total = 0;
	for (const p of prefixes) {
		let node = root;
		for (let i = 0; i < p.length && node !== null; i++) {
			node = node.children[p.charCodeAt(i) - 97];
		}
		if (node !== null) total += node.count;
	}
	return `${nodes}:${total}`;
}

function lruKeys(n) {
	const capacity = Math.max(1, Math.floor(n / 100));
	const keys = new XorShift32().ints(n).map((k) => k % (2 * capacity));
	return { capacity, keys };
}

// A Map finds the node and a doubly linked list keeps the order, newest at the head.
class LruCache {
	nodes = new Map();
	head = null;
	tail = null;

	unlink(node) {
		if (node.prev === null) this.head = node.next;
		else node.prev.next = node.next;
		if (node.next === null) this.tail = node.prev;
		else node.next.prev = node.prev;
	}

	pushFront(node) {
		node.prev = null;
		node.next = this.head;
		if (this.head === null) this.tail = node;
		else this.head.prev = node;
		this.head = node;
	}

	*keysOldestFirst() {
		for (let node = this.tail; node !== null; node = node.prev) yield node.key;
	}
}

function lru({ capacity, keys }) {
	const cache = new LruCache();
	let hits = 0;
	for (let i = 0; i < keys.length; i++) {
		const k = keys[i];
		const found = cache.nodes.get(k);
		if (found !== undefined) {
			cache.unlink(found);
			cache.pushFront(found);
			hits++;
			continue;
		}
		if (cache.nodes.size === capacity) {
			const oldest = cache.tail;
			cache.unlink(oldest);
			cache.nodes.delete(oldest.key);
		}
		const node = { key: k, value: i, prev: null, next: null };
		cache.nodes.set(k, node);
		cache.pushFront(node);
	}
	return `${hits}:${keys.length - hits}:${h31(cache.keysOldestFirst())}`;
}

function dna(length, count) {
	const rng = new XorShift32();
	const out = [];
	for (let s = 0; s < count; s++) {
		const bytes = new Uint8Array(length);
		for (let i = 0; i < length; i++) bytes[i] = ACGT[rng.next() % 4];
		out.push(new TextDecoder("latin1").decode(bytes));
	}
	return out;
}

function editDistance([a, b]) {
	let prev = new Int32Array(b.length + 1);
	let cur = new Int32Array(b.length + 1);
	for (let j = 0; j <= b.length; j++) prev[j] = j;
	for (let i = 0; i < a.length; i++) {
		cur[0] = i + 1;
		const ca = a.charCodeAt(i);
		for (let j = 0; j < b.length; j++) {
			const cost = ca === b.charCodeAt(j) ? 0 : 1;
			cur[j + 1] = Math.min(prev[j + 1] + 1, cur[j] + 1, prev[j] + cost);
		}
		[prev, cur] = [cur, prev];
	}
	return String(prev[b.length]);
}

function items(capacity) {
	const rng = new XorShift32();
	const things = [];
	for (let i = 0; i < KNAPSACK_ITEMS; i++) {
		const weight = 1 + (rng.next() % 1000);
		things.push({ weight, value: 1 + (rng.next() % 1000) });
	}
	return { capacity, things };
}

function knapsack({ capacity, things }) {
	const best = new Float64Array(capacity + 1);
	for (const { weight, value } of things) {
		for (let c = capacity; c >= weight; c--) {
			if (best[c - weight] + value > best[c])
				best[c] = best[c - weight] + value;
		}
	}
	return `${best[capacity]}:${h31(best)}`;
}

// JavaScript has no standard binary search: the usual lower bound.
function lowerBound(a, x) {
	let lo = 0;
	let hi = a.length;
	while (lo < hi) {
		const mid = (lo + hi) >>> 1;
		if (a[mid] < x) lo = mid + 1;
		else hi = mid;
	}
	return lo;
}

function lis(values) {
	const tails = [];
	for (const x of values) {
		const i = lowerBound(tails, x);
		if (i === tails.length) tails.push(x);
		else tails[i] = x;
	}
	return `${tails.length}:${h31(tails)}`;
}

// The 64 most common English words (kernels/SPEC.md section 12, kmp-en).
const WORDS = (
	"the be to of and a in that have i it for not on with he as you do at this but his by from they we say her " +
	"she or an will my one all would there their what so up out if about who get which go me when make can like " +
	"time no just him know take people into year your"
).split(" ");

function english(n) {
	const rng = new XorShift32();
	const parts = [];
	let size = 0;
	while (size < n) {
		const a = rng.next() % 64;
		const b = rng.next() % 64;
		const word = WORDS[Math.min(a, b)];
		const r = rng.next() % 16;
		const sep = r === 0 ? ". " : r === 1 ? ", " : " ";
		parts.push(word, sep);
		size += word.length + sep.length;
	}
	return parts.join("").slice(0, n);
}

function textEn(n) {
	const t = english(n);
	const mid = Math.floor(n / 2);
	return { text: t, pattern: t.slice(mid, mid + PATTERN_LENGTH) };
}

function text(n) {
	const t = dna(n, 1)[0];
	const mid = Math.floor(n / 2);
	return { text: t, pattern: t.slice(mid, mid + PATTERN_LENGTH) };
}

function kmp({ text: t, pattern: p }) {
	const m = p.length;
	const fail = new Int32Array(m);
	let k = 0;
	for (let i = 1; i < m; i++) {
		while (k > 0 && p.charCodeAt(i) !== p.charCodeAt(k)) k = fail[k - 1];
		if (p.charCodeAt(i) === p.charCodeAt(k)) k++;
		fail[i] = k;
	}
	let count = 0;
	let first = -1;
	let last = -1;
	k = 0;
	for (let i = 0; i < t.length; i++) {
		const ch = t.charCodeAt(i);
		while (k > 0 && ch !== p.charCodeAt(k)) k = fail[k - 1];
		if (ch === p.charCodeAt(k)) k++;
		if (k === m) {
			last = i + 1 - m;
			if (first < 0) first = last;
			count++;
			k = fail[k - 1];
		}
	}
	return `${count}:${first}:${last}`;
}

function strFind({ text: t, pattern: p }) {
	let count = 0;
	let first = -1;
	let last = -1;
	for (let pos = t.indexOf(p); pos >= 0; pos = t.indexOf(p, pos + 1)) {
		if (first < 0) first = pos;
		last = pos;
		count++;
	}
	return `${count}:${first}:${last}`;
}

const fib = (n) => (n < 2 ? n : fib(n - 1) + fib(n - 2));

function nqueens(n) {
	const cols = new Array(n).fill(false);
	const up = new Array(2 * n - 1).fill(false);
	const down = new Array(2 * n - 1).fill(false);
	const place = (row) => {
		if (row === n) return 1;
		let count = 0;
		for (let col = 0; col < n; col++) {
			const u = row + col;
			const d = row + n - 1 - col;
			if (!cols[col] && !up[u] && !down[d]) {
				cols[col] = up[u] = down[d] = true;
				count += place(row + 1);
				cols[col] = up[u] = down[d] = false;
			}
		}
		return count;
	};
	return place(0);
}

function sortedArray(n) {
	const rng = new XorShift32();
	const a = new Array(n);
	let v = 0;
	for (let i = 0; i < n; i++) {
		v += 1 + (rng.next() % 4);
		a[i] = v;
	}
	const queries = new Array(n);
	for (let i = 0; i < n; i++) queries[i] = rng.next() % (v + 1);
	return { a, queries };
}

function binarySearch({ a, queries }) {
	const n = a.length;
	let hits = 0;
	let total = 0;
	for (const q of queries) {
		let lo = 0;
		let hi = n;
		while (lo < hi) {
			const mid = (lo + hi) >>> 1;
			if (a[mid] < q) lo = mid + 1;
			else hi = mid;
		}
		if (lo < n && a[lo] === q) hits++;
		total += lo;
	}
	return `${hits}:${total}`;
}

const copyEdges = ({ n, edges }) => ({ n, edges: edges.slice() });

export const ALGO_KERNELS = {
	"bfs-grid": { prepare: grid, run: bfsGrid, check: String },
	dijkstra: { prepare: graph, run: dijkstra, check: String },
	kruskal: { prepare: edgeList, fresh: copyEdges, run: kruskal, check: String },
	bst: { prepare: hashmapInput, run: bst, check: String },
	trie: { prepare: words, run: trie, check: String },
	lru: { prepare: lruKeys, run: lru, check: String },
	"edit-distance": {
		prepare: (n) => dna(n, 2),
		run: editDistance,
		check: String,
	},
	knapsack: { prepare: items, run: knapsack, check: String },
	lis: { prepare: (n) => new XorShift32().ints(n), run: lis, check: String },
	kmp: { prepare: text, run: kmp, check: String },
	"str-find": { prepare: text, run: strFind, check: String },
	"kmp-en": { prepare: textEn, run: kmp, check: String },
	"str-find-en": { prepare: textEn, run: strFind, check: String },
	fib: { run: fib, check: String },
	nqueens: { run: nqueens, check: String },
	"binary-search": { prepare: sortedArray, run: binarySearch, check: String },
};
