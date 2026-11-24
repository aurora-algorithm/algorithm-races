// txiki.js (`tjs run`): QuickJS-ng + libuv.

import "./polyfill.mjs";
import { start } from "./runner.mjs";

const { tjs } = globalThis;

start({
	args: () => tjs.args.slice(3),
	readText: async (path) =>
		new TextDecoder("utf-8").decode(await tjs.readFile(path)),
	print: (line) => console.log(line),
	printError: (line) => console.error(line),
	runtimeInfo: () =>
		`txiki.js ${tjs.version} quickjs-ng ${tjs.engine.versions.quickjs}`,
	exit: (code) => {
		if (code !== 0) tjs.exit(code);
	},
});
