// Race: trie (js)
// Extracted from suite/js/algo.mjs — build and run via suite/run.py

import { h31, XorShift32 } from "./gen.mjs";

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
