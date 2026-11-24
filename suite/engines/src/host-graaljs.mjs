// Standalone GraalJS (`js FILE -- ARGS`): Graal's JS engine without Node's APIs.

import "./polyfill.mjs";
import { start } from "./runner.mjs";

const g = globalThis;

start({
	args: () => Array.from(g.arguments),
	readText: (path) => g.read(path),
	print: (line) => g.print(line),
	printError: (line) => g.printErr(line),
	runtimeInfo: () => `GraalJS ${g.Graal.versionGraalVM} standalone`,
	exit: (code) => {
		if (code !== 0) g.quit(code);
	},
});
