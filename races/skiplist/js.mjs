// Race: skiplist (js)
// Extracted from suite/js/puzzle.mjs — build and run via suite/run.py

import { fnv1a, h31, XorShift32 } from "./gen.mjs";

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
