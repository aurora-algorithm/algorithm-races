// Race: mandelbrot-mt (js)
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

export async function mandelbrotMt(n, pool) {
	const out = new SharedArrayBuffer(n * rowBytes(n));
	await Promise.all(
		pool.workers.map((_, t) =>
			pool.run(t, "mandelbrot", { out, n, first: t, step: pool.size }),
		),
	);
	return new Uint8Array(out);
}
