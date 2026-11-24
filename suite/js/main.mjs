// Kernel runner (kernels/SPEC.md section 1). Runs unchanged on Node, Bun and Deno.
// usage: main.mjs KERNEL --size N [--repeats R] [--threads T] [--data DIR]

import { readFileSync } from "node:fs";
import { join } from "node:path";
import { parseArgs } from "node:util";
import { ALGO_KERNELS } from "./algo.mjs";
import { ARENA_KERNELS } from "./arena.mjs";
import * as bg from "./bg.mjs";
import {
	fnv1a,
	h31,
	logText,
	matrices,
	randomStrings,
	shaBuffer,
	utf8,
	XorShift32,
} from "./gen.mjs";
import * as misc from "./misc.mjs";
import * as mt from "./mt.mjs";
import { PUZZLE_KERNELS } from "./puzzle.mjs";
import * as sorts from "./sorts.mjs";
import { WAYS_KERNELS } from "./ways.mjs";

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

function sharedCopy(values) {
	const data = new SharedArrayBuffer(
		values.length * Float64Array.BYTES_PER_ELEMENT,
	);
	new Float64Array(data).set(values);
	return data;
}

const sortKernel = (run) => ({
	prepare: ints,
	fresh: copy,
	run,
	check: sortedInts,
});

const KERNELS = {
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
		prepare: (n, dir) => readFileSync(join(dir, `orders-${n}.json`), "utf8"),
		run: misc.jsonRoundtrip,
		check: misc.jsonChecksum,
	},
	"mergesort-mt": {
		prepare: ints,
		fresh: sharedCopy,
		run: mt.mergesortMt,
		check: sortedInts,
		threaded: true,
	},
	"mandelbrot-mt": {
		run: mt.mandelbrotMt,
		check: bytesChecksum,
		threaded: true,
	},
	"binary-trees-mt": {
		run: mt.binaryTreesMt,
		check: textChecksum,
		threaded: true,
	},
};

function runtimeInfo() {
	if (globalThis.Deno) return `deno ${Deno.version.deno} v8 ${Deno.version.v8}`;
	if (process.versions.bun) return `bun ${process.versions.bun} JavaScriptCore`;
	return `node ${process.version} v8 ${process.versions.v8}`;
}

function positiveInt(name, text) {
	const value = Number(text);
	if (!Number.isSafeInteger(value) || value < 1)
		throw new Error(`--${name} must be a positive integer, got ${text}`);
	return value;
}

function parseCli(argv) {
	const { values, positionals } = parseArgs({
		args: argv,
		allowPositionals: true,
		options: {
			size: { type: "string" },
			repeats: { type: "string", default: "1" },
			threads: { type: "string", default: "1" },
			data: { type: "string", default: "." },
		},
	});
	if (positionals.length !== 1 || values.size === undefined) {
		throw new Error(
			"usage: main.mjs KERNEL --size N [--repeats R] [--threads T] [--data DIR]",
		);
	}
	return {
		kernel: positionals[0],
		size: positiveInt("size", values.size),
		repeats: positiveInt("repeats", values.repeats),
		threads: positiveInt("threads", values.threads),
		data: values.data,
	};
}

async function measure(kernel, args, pool) {
	const base = (kernel.prepare ?? ((n) => n))(args.size, args.data);
	const fresh = kernel.fresh ?? ((input) => input);
	const times = [];
	let result;
	for (let r = 0; r < args.repeats; r++) {
		const input = fresh(base);
		const t0 = performance.now();
		result = kernel.threaded
			? await kernel.run(input, pool)
			: kernel.async
				? await kernel.run(input)
				: kernel.run(input);
		times.push(Math.round((performance.now() - t0) * 1000) / 1000);
	}
	const checked = await kernel.check(result);
	return typeof checked === "string"
		? { checksum: checked, times }
		: { ...checked, times };
}

async function findKernel(name) {
	if (KERNELS[name]) return { kernel: KERNELS[name] };
	if (ALGO_KERNELS[name]) return { kernel: ALGO_KERNELS[name] };
	if (PUZZLE_KERNELS[name]) return { kernel: PUZZLE_KERNELS[name] };
	if (WAYS_KERNELS[name]) return { kernel: WAYS_KERNELS[name] };
	if (ARENA_KERNELS[name]) return { kernel: ARENA_KERNELS[name] };
	const work = await import("./work.mjs");
	return { kernel: work.WORK_KERNELS[name], libraries: work.libraries };
}

async function main() {
	const args = parseCli(process.argv.slice(2));
	const { kernel, libraries } = await findKernel(args.kernel);
	if (!kernel) {
		console.error(`unknown kernel ${args.kernel}`);
		return 2;
	}
	const pool = kernel.threaded
		? await new mt.WorkerPool(args.threads).ready()
		: null;
	try {
		const { checksum, times, extra } = await measure(kernel, args, pool);
		const row = {
			kernel: args.kernel,
			size: args.size,
			threads: args.threads,
			checksum,
			times_ms: times,
		};
		const runtime = libraries
			? `${runtimeInfo()} ${libraries()}`
			: runtimeInfo();
		console.log(JSON.stringify({ ...row, runtime, extra }));
	} finally {
		await pool?.close();
	}
	return 0;
}

try {
	process.exitCode = await main();
} catch (err) {
	console.error(err.message);
	process.exitCode = 1;
}
