// The -arena kernels (kernels/SPEC.md section 12, "Arena rows"): bst, trie and binary-trees with every node in
// growable typed arrays and 32-bit index links, the same algorithms and checksums as the object versions.

import { hashmapInput, words } from "./algo.mjs";
import { depthLine, iterationsFor, MIN_DEPTH, maxDepthFor } from "./bg.mjs";
import { fnv1a, h31, utf8 } from "./gen.mjs";

const FIRST_CAPACITY = 16;
// The "no child" link of the BST and the binary trees.
const NIL = -1;

function grown(array, length) {
	const bigger = new array.constructor(length);
	bigger.set(array);
	return bigger;
}

// One BST node per index in four parallel typed arrays, grown together. push may replace the arrays, so a
// caller reads t.left or t.right only after it returns.
class BstNodes {
	key = new Float64Array(FIRST_CAPACITY);
	count = new Int32Array(FIRST_CAPACITY);
	left = new Int32Array(FIRST_CAPACITY);
	right = new Int32Array(FIRST_CAPACITY);
	size = 0;

	push(k) {
		if (this.size === this.key.length) {
			this.key = grown(this.key, 2 * this.size);
			this.count = grown(this.count, 2 * this.size);
			this.left = grown(this.left, 2 * this.size);
			this.right = grown(this.right, 2 * this.size);
		}
		this.key[this.size] = k;
		this.count[this.size] = 1;
		this.left[this.size] = NIL;
		this.right[this.size] = NIL;
		return this.size++;
	}
}

function inOrder(t) {
	const keys = new Float64Array(t.size);
	let i = 0;
	const stack = [];
	let cur = t.size === 0 ? NIL : 0;
	while (cur !== NIL || stack.length > 0) {
		while (cur !== NIL) {
			stack.push(cur);
			cur = t.left[cur];
		}
		cur = stack.pop();
		keys[i++] = t.key[cur];
		cur = t.right[cur];
	}
	return keys;
}

function bstArena({ inserts, lookups }) {
	const t = new BstNodes();
	for (const k of inserts) {
		if (t.size === 0) {
			t.push(k);
			continue;
		}
		let cur = 0;
		for (;;) {
			if (k < t.key[cur]) {
				if (t.left[cur] === NIL) {
					const fresh = t.push(k);
					t.left[cur] = fresh;
					break;
				}
				cur = t.left[cur];
			} else if (k > t.key[cur]) {
				if (t.right[cur] === NIL) {
					const fresh = t.push(k);
					t.right[cur] = fresh;
					break;
				}
				cur = t.right[cur];
			} else {
				t.count[cur]++;
				break;
			}
		}
	}
	let hits = 0;
	for (const k of lookups) {
		let cur = t.size === 0 ? NIL : 0;
		while (cur !== NIL && t.key[cur] !== k) {
			cur = k < t.key[cur] ? t.left[cur] : t.right[cur];
		}
		if (cur !== NIL) hits++;
	}
	return `${t.size}:${hits}:${h31(inOrder(t))}`;
}

// A trie node is 27 ints at node * 27: the count, then 26 child links. Node 0 is the root and never anyone's
// child, so a child link of 0 means no child.
const TRIE_STRIDE = 27;

function trieArena({ inserted, prefixes }) {
	let nodes = new Int32Array(FIRST_CAPACITY * TRIE_STRIDE);
	let count = 1;
	for (const w of inserted) {
		let node = 0;
		for (let i = 0; i < w.length; i++) {
			const slot = node * TRIE_STRIDE + 1 + w.charCodeAt(i) - 97;
			if (nodes[slot] === 0) {
				if ((count + 1) * TRIE_STRIDE > nodes.length) {
					nodes = grown(nodes, 2 * nodes.length);
				}
				nodes[slot] = count++;
			}
			node = nodes[slot];
			nodes[node * TRIE_STRIDE]++;
		}
	}
	let total = 0;
	for (const p of prefixes) {
		// A prefix has at least one letter, so node 0 after the walk means it is absent.
		let node = 0;
		for (let i = 0; i < p.length; i++) {
			node = nodes[node * TRIE_STRIDE + 1 + p.charCodeAt(i) - 97];
			if (node === 0) break;
		}
		if (node !== 0) total += nodes[node * TRIE_STRIDE];
	}
	return `${count - 1}:${total}`;
}

// One binary tree: node i's links at 2i (left) and 2i + 1 (right), children pushed before their parent; a
// leaf's links are NIL.
class TreeArena {
	links = new Int32Array(2 * FIRST_CAPACITY);
	count = 0;

	make(depth) {
		let left = NIL;
		let right = NIL;
		if (depth > 0) {
			left = this.make(depth - 1);
			right = this.make(depth - 1);
		}
		if (2 * this.count === this.links.length) {
			this.links = grown(this.links, 2 * this.links.length);
		}
		this.links[2 * this.count] = left;
		this.links[2 * this.count + 1] = right;
		return this.count++;
	}

	check(node) {
		const left = this.links[2 * node];
		if (left === NIL) return 1;
		return 1 + this.check(left) + this.check(this.links[2 * node + 1]);
	}
}

function treeCheck(depth) {
	const tree = new TreeArena();
	return tree.check(tree.make(depth));
}

function binaryTreesArena(n) {
	const maxDepth = maxDepthFor(n);
	const stretch = maxDepth + 1;
	const lines = [
		`stretch tree of depth ${stretch}\t check: ${treeCheck(stretch)}`,
	];
	const longLived = new TreeArena();
	const longRoot = longLived.make(maxDepth);
	for (let depth = MIN_DEPTH; depth <= maxDepth; depth += 2) {
		const iterations = iterationsFor(maxDepth, depth);
		let check = 0;
		for (let i = 0; i < iterations; i++) check += treeCheck(depth);
		lines.push(depthLine(iterations, depth, check));
	}
	lines.push(
		`long lived tree of depth ${maxDepth}\t check: ${longLived.check(longRoot)}`,
	);
	return `${lines.join("\n")}\n`;
}

export const ARENA_KERNELS = {
	"bst-arena": { prepare: hashmapInput, run: bstArena, check: String },
	"trie-arena": { prepare: words, run: trieArena, check: String },
	"binary-trees-arena": {
		run: binaryTreesArena,
		check: (text) => String(fnv1a(utf8(text))),
	},
};
