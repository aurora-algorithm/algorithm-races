// Race: binary-trees-mt (js)
// Extracted from suite/js/mt.mjs — build and run via suite/run.py

import {
	depthLine,
	iterationsFor,
	longLivedLine,
	MIN_DEPTH,
	makeTree,
	maxDepthFor,
	rowBytes,
	stretchLine,
} from "./bg.mjs";

async function depthChecksMt(depth, iterations, pool) {
	const share = iterations / pool.size;
	const checks = await Promise.all(
		pool.workers.map((_, t) =>
			pool.run(t, "trees", { depth, iterations: share }),
		),
	);
	return checks.reduce((sum, c) => sum + c, 0);
}

export async function binaryTreesMt(n, pool) {
	const maxDepth = maxDepthFor(n);
	const lines = [stretchLine(maxDepth + 1)];
	const longLived = makeTree(maxDepth);
	for (let depth = MIN_DEPTH; depth <= maxDepth; depth += 2) {
		const iterations = iterationsFor(maxDepth, depth);
		lines.push(
			depthLine(
				iterations,
				depth,
				await depthChecksMt(depth, iterations, pool),
			),
		);
	}
	lines.push(longLivedLine(maxDepth, longLived));
	return `${lines.join("\n")}\n`;
}
