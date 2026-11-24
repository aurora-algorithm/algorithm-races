// QuickJS-ng's qjs, run with --std so `std` is a global (the bundle has no imports).

import "./polyfill.mjs";
import { start } from "./runner.mjs";

const { std } = globalThis;

start({
	args: () => globalThis.scriptArgs.slice(1),
	readText: (path) => {
		const text = std.loadFile(path);
		if (text === null) throw new Error(`cannot read ${path}`);
		return text;
	},
	print: (line) => {
		std.out.puts(`${line}\n`);
		std.out.flush();
	},
	printError: (line) => {
		std.err.puts(`${line}\n`);
		std.err.flush();
	},
	runtimeInfo: () => `${globalThis.navigator.userAgent} interpreter`,
	exit: (code) => {
		if (code !== 0) std.exit(code);
	},
});
