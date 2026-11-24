// Race: wordcount (js)
// Extracted from suite/js/work.mjs — build and run via suite/run.py

import { createReadStream, readFileSync } from "node:fs";
import { createInterface } from "node:readline";

function top(map, compareKeys) {
	let best;
	for (const entry of map) {
		if (
			best === undefined ||
			entry[1] > best[1] ||
			(entry[1] === best[1] && compareKeys(entry[0], best[0]) < 0)
		)
			best = entry;
	}
	return best;
}

async function wordcount(path) {
	const counts = new Map();
	const lines = createInterface({
		input: createReadStream(path),
		crlfDelay: Number.POSITIVE_INFINITY,
	});
	for await (const line of lines) {
		for (const word of line.split(" ")) {
			if (word) counts.set(word, (counts.get(word) ?? 0) + 1);
		}
	}
	return counts;
}

function wordcountChecksum(counts) {
	let total = 0;
	for (const v of counts.values()) total += v;
	const [word, count] = top(counts, (a, b) => (a < b ? -1 : a > b ? 1 : 0));
	return `${total}:${counts.size}:${word}:${count}`;
}
