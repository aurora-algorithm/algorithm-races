// One worker of the pool in mt.mjs: runs the task it is sent and posts the result back.

import { parentPort } from "node:worker_threads";
import { depthChecks, mandelbrotRow } from "./bg.mjs";
import { merge, mergeSort } from "./sorts.mjs";

const tasks = {
	mergesort({ data, aux, lo, hi }) {
		mergeSort(new Float64Array(data), new Float64Array(aux), lo, hi);
	},
	merge({ data, aux, lo, mid, hi }) {
		merge(new Float64Array(data), new Float64Array(aux), lo, mid, hi);
	},
	mandelbrot({ out, n, first, step }) {
		const rows = new Uint8Array(out);
		for (let y = first; y < n; y += step) mandelbrotRow(n, y, rows);
	},
	trees({ depth, iterations }) {
		return depthChecks(depth, iterations);
	},
};

parentPort.on("message", ({ task, args }) => {
	parentPort.postMessage(tasks[task](args) ?? null);
});
parentPort.postMessage("ready");
