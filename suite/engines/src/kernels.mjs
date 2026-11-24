// The single-threaded kernels of kernels/js/main.mjs, imported from the same modules (no worker_threads, no work
// group), so every engine runs byte-for-byte the code Node, Bun and Deno run.

import { ALGO_KERNELS } from "../../js/algo.mjs";
import * as bg from "../../js/bg.mjs";
import {
	fnv1a,
	h31,
	logText,
	matrices,
	randomStrings,
	shaBuffer,
	utf8,
	XorShift32,
} from "../../js/gen.mjs";
import * as misc from "../../js/misc.mjs";
import { PUZZLE_KERNELS } from "../../js/puzzle.mjs";
import * as sorts from "../../js/sorts.mjs";

const sortedInts = (a) => String(h31(a));
const stringsChecksum = (a) =>
	String(fnv1a(utf8(a.map((s) => `${s}\n`).join(""))));
const bytesChecksum = (bytes) => String(fnv1a(bytes));
const textChecksum = (text) => String(fnv1a(utf8(text)));
const ints = (n) => new XorShift32().ints(n);
const copy = (a) => a.slice();

function hashmapInput(n) {
	const inserts = ints(n);
	return { inserts, lookups: inserts.map((k, i) => (k ^ (i & 1)) >>> 0) };
}

const sortKernel = (run) => ({
	prepare: ints,
	fresh: copy,
	run,
	check: sortedInts,
});

export const KERNELS = {
	"sort-int": sortKernel((a) => a.sort((x, y) => x - y)),
	"sort-str": {
		prepare: randomStrings,
		fresh: copy,
		run: (a) => a.sort(),
		check: stringsChecksum,
	},
	quicksort: sortKernel(sorts.runQuicksort),
	mergesort: sortKernel(sorts.runMergesort),
	heapsort: sortKernel(sorts.heapsort),
	radixsort: sortKernel(sorts.radixSort),
	"binary-trees": { run: bg.binaryTrees, check: textChecksum },
	nbody: { run: bg.nbody, check: String },
	mandelbrot: { run: bg.mandelbrot, check: bytesChecksum },
	fannkuch: { run: bg.fannkuch, check: String },
	"spectral-norm": { run: bg.spectralNorm, check: String },
	"regex-log": { prepare: logText, run: misc.regexLog, check: String },
	hashmap: {
		prepare: hashmapInput,
		run: (m) => misc.hashmap(m.inserts, m.lookups),
		check: String,
	},
	sha256: { prepare: shaBuffer, run: misc.sha256, check: String },
	sieve: { run: misc.sieve, check: String },
	matmul: { prepare: matrices, run: misc.matmul, check: misc.matmulChecksum },
	json: {
		prepare: (n, readText, dir) => readText(`${dir}/orders-${n}.json`),
		run: misc.jsonRoundtrip,
		check: misc.jsonChecksum,
	},
	...ALGO_KERNELS,
	...PUZZLE_KERNELS,
};
