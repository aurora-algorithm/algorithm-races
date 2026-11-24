// Race: trie-arena (js)
// Extracted from suite/js/arena.mjs — build and run via suite/run.py

const FIRST_CAPACITY = 16;

function grown(array, length) {
	const bigger = new array.constructor(length);
	bigger.set(array);
	return bigger;
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
