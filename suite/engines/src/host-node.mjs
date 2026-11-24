// Node (the control: the same bundle as the other engines) and LLRT, which implements node:fs and process.

import { readFileSync } from "node:fs";
import { start } from "./runner.mjs";

function runtimeInfo() {
	const v = process.versions;
	if (v.llrt) return `llrt ${v.llrt} QuickJS (portable bundle)`;
	return `node ${process.version} v8 ${v.v8} (portable bundle)`;
}

await start({
	args: () => process.argv.slice(2),
	readText: (path) => readFileSync(path, "utf8"),
	print: (line) => console.log(line),
	printError: (line) => console.error(line),
	runtimeInfo,
	exit: (code) => {
		process.exitCode = code;
	},
});
