// Race: bst-arena (js)
// Extracted from suite/js/arena.mjs — build and run via suite/run.py

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
