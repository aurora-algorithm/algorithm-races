// The kernels/SPEC.md section 1 contract on any engine: `KERNEL --size N [--repeats R] [--threads T] [--data DIR]`,
// one JSON line on stdout. The engine-specific parts (arguments, file reads, output, exit) come from a host module.

import { KERNELS } from "./kernels.mjs";

const USAGE = "usage: KERNEL --size N [--repeats R] [--threads T] [--data DIR]";

function positiveInt(name, text) {
	const value = Number(text);
	if (!Number.isSafeInteger(value) || value < 1)
		throw new Error(`--${name} must be a positive integer, got ${text}`);
	return value;
}

export function parseCli(argv) {
	const options = { repeats: "1", threads: "1", data: "." };
	const positionals = [];
	for (let i = 0; i < argv.length; i++) {
		const name = argv[i].startsWith("--") ? argv[i].slice(2) : null;
		if (name === null) {
			positionals.push(argv[i]);
		} else if (["size", "repeats", "threads", "data"].includes(name)) {
			if (i + 1 >= argv.length) throw new Error(USAGE);
			options[name] = argv[++i];
		} else {
			throw new Error(`unknown option --${name}`);
		}
	}
	if (positionals.length !== 1 || options.size === undefined)
		throw new Error(USAGE);
	return {
		kernel: positionals[0],
		size: positiveInt("size", options.size),
		repeats: positiveInt("repeats", options.repeats),
		threads: positiveInt("threads", options.threads),
		data: options.data,
	};
}

async function measure(kernel, args, readText) {
	const prepare = kernel.prepare ?? ((n) => n);
	const base = await prepare(args.size, readText, args.data);
	const fresh = kernel.fresh ?? ((input) => input);
	const times = [];
	let result;
	for (let r = 0; r < args.repeats; r++) {
		const input = fresh(base);
		const t0 = performance.now();
		result = kernel.async ? await kernel.run(input) : kernel.run(input);
		times.push(Math.round((performance.now() - t0) * 1000) / 1000);
	}
	const checked = await kernel.check(result);
	return typeof checked === "string"
		? { checksum: checked, times }
		: { ...checked, times };
}

export async function run(host) {
	const args = parseCli(host.args());
	const kernel = KERNELS[args.kernel];
	if (!kernel || args.threads !== 1) {
		host.printError(`unknown kernel ${args.kernel} (threads ${args.threads})`);
		return 2;
	}
	const { checksum, times, extra } = await measure(kernel, args, host.readText);
	const row = {
		kernel: args.kernel,
		size: args.size,
		threads: args.threads,
		checksum,
		times_ms: times,
		runtime: host.runtimeInfo(),
		extra,
	};
	host.print(JSON.stringify(row));
	return 0;
}

export async function start(host) {
	let code;
	try {
		code = await run(host);
	} catch (err) {
		host.printError(String(err?.message ?? err));
		code = 1;
	}
	host.exit(code);
}
