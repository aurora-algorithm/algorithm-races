// Multi-threaded kernels (kernels/SPEC.md section 6) on a pool of worker_threads started before timing.

import { Worker } from "node:worker_threads";
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

class PooledWorker {
	#worker;
	#waiting = [];

	constructor(url) {
		this.#worker = new Worker(url);
		this.#worker.on("message", (result) =>
			this.#waiting.shift().resolve(result),
		);
		this.#worker.on("error", (err) => {
			for (const job of this.#waiting.splice(0)) job.reject(err);
		});
		this.ready = this.#reply();
	}

	#reply() {
		return new Promise((resolve, reject) =>
			this.#waiting.push({ resolve, reject }),
		);
	}

	run(task, args) {
		const reply = this.#reply();
		this.#worker.postMessage({ task, args });
		return reply;
	}

	terminate() {
		return this.#worker.terminate();
	}
}

export class WorkerPool {
	constructor(size) {
		const url = new URL("./worker.mjs", import.meta.url);
		this.workers = Array.from({ length: size }, () => new PooledWorker(url));
	}

	get size() {
		return this.workers.length;
	}

	async ready() {
		await Promise.all(this.workers.map((w) => w.ready));
		return this;
	}

	run(index, task, args) {
		return this.workers[index].run(task, args);
	}

	close() {
		return Promise.all(this.workers.map((w) => w.terminate()));
	}
}

export const chunkBounds = (n, threads) =>
	Array.from({ length: threads + 1 }, (_, t) => Math.floor((t * n) / threads));

// `data` is a SharedArrayBuffer of Float64 values; the sorted result is left in it.
export async function mergesortMt(data, pool) {
	const aux = new SharedArrayBuffer(data.byteLength);
	let bounds = chunkBounds(
		data.byteLength / Float64Array.BYTES_PER_ELEMENT,
		pool.size,
	);
	await Promise.all(
		pool.workers.map((_, t) =>
			pool.run(t, "mergesort", { data, aux, lo: bounds[t], hi: bounds[t + 1] }),
		),
	);
	while (bounds.length > 2) {
		const merges = [];
		for (let p = 0; p + 2 < bounds.length; p += 2) {
			const args = {
				data,
				aux,
				lo: bounds[p],
				mid: bounds[p + 1],
				hi: bounds[p + 2],
			};
			merges.push(pool.run(p / 2, "merge", args));
		}
		await Promise.all(merges);
		bounds = bounds.filter((_, i) => i % 2 === 0 || i === bounds.length - 1);
	}
	return new Float64Array(data);
}

export async function mandelbrotMt(n, pool) {
	const out = new SharedArrayBuffer(n * rowBytes(n));
	await Promise.all(
		pool.workers.map((_, t) =>
			pool.run(t, "mandelbrot", { out, n, first: t, step: pool.size }),
		),
	);
	return new Uint8Array(out);
}

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
