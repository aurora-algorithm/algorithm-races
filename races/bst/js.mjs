// Race: bst (js)
// Extracted from suite/js/algo.mjs — build and run via suite/run.py

import { h31, XorShift32 } from "./gen.mjs";

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
