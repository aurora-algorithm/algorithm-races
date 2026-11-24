// Race: binary-trees (js)
// Extracted from suite/js/bg.mjs — build and run via suite/run.py

export const MIN_DEPTH = 4;

class TreeNode {
	constructor(left, right) {
		this.left = left;
		this.right = right;
	}
}

export function makeTree(depth) {
	if (depth === 0) return new TreeNode(null, null);
	return new TreeNode(makeTree(depth - 1), makeTree(depth - 1));
}

export function checkTree(node) {
	if (node.left === null) return 1;
	return 1 + checkTree(node.left) + checkTree(node.right);
}

export function depthChecks(depth, iterations) {
	let check = 0;
	for (let i = 0; i < iterations; i++) check += checkTree(makeTree(depth));
	return check;
}

export const maxDepthFor = (n) => Math.max(MIN_DEPTH + 2, n);

export const iterationsFor = (maxDepth, depth) =>
	1 << (maxDepth - depth + MIN_DEPTH);

export const stretchLine = (depth) =>
	`stretch tree of depth ${depth}\t check: ${checkTree(makeTree(depth))}`;

export const depthLine = (iterations, depth, check) =>
	`${iterations}\t trees of depth ${depth}\t check: ${check}`;

export const longLivedLine = (depth, tree) =>
	`long lived tree of depth ${depth}\t check: ${checkTree(tree)}`;

export function binaryTrees(n) {
	const maxDepth = maxDepthFor(n);
	const lines = [stretchLine(maxDepth + 1)];
	const longLived = makeTree(maxDepth);
	for (let depth = MIN_DEPTH; depth <= maxDepth; depth += 2) {
		const iterations = iterationsFor(maxDepth, depth);
		lines.push(depthLine(iterations, depth, depthChecks(depth, iterations)));
	}
	lines.push(longLivedLine(maxDepth, longLived));
	return `${lines.join("\n")}\n`;
}
