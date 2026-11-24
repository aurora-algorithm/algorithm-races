// Race: binary-trees-arena (js)
// Extracted from suite/js/arena.mjs — build and run via suite/run.py

import { depthLine, iterationsFor, MIN_DEPTH, maxDepthFor } from "./bg.mjs";

const FIRST_CAPACITY = 16;

// The "no child" link of the BST and the binary trees.
const NIL = -1;

function grown(array, length) {
	const bigger = new array.constructor(length);
	bigger.set(array);
	return bigger;
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
