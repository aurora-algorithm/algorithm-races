// Bundles the kernels into one file per engine (dist/<engine>.js) with esbuild: no minify, no transforms beyond
// joining the modules, so the engines run the same kernel code as kernels/js.

import { build } from "esbuild";

const shim = { "node:crypto": "./src/crypto-shim.mjs" };

const TARGETS = [
	{ name: "node", host: "host-node.mjs", format: "esm", alias: {} },
	{ name: "llrt", host: "host-node.mjs", format: "esm", alias: {} },
	{ name: "qjs", host: "host-qjs.mjs", format: "iife", alias: shim },
	{ name: "tjs", host: "host-tjs.mjs", format: "iife", alias: shim },
	{ name: "graaljs", host: "host-graaljs.mjs", format: "iife", alias: shim },
];

for (const t of TARGETS) {
	await build({
		entryPoints: [`src/${t.host}`],
		outfile: `dist/${t.name}.${t.format === "esm" ? "mjs" : "js"}`,
		bundle: true,
		format: t.format,
		platform: "neutral",
		target: "es2022",
		external: ["node:*"],
		alias: t.alias,
		logLevel: "warning",
	});
}
