// Race: bmap-int.go.map (js)
// Extracted from suite/js/ways.mjs — build and run via suite/run.py

import { fnv1a, h31, randomStrings, utf8, XorShift32 } from "./gen.mjs";

const ints = (n) => new XorShift32().ints(n);

function mapIntInput(n) {
	const inserts = ints(n);
	return { inserts, lookups: inserts.map((k, i) => (k ^ (i & 1)) >>> 0) };
}

function countMap({ inserts, lookups }) {
	const counts = new Map();
	for (const k of inserts) counts.set(k, (counts.get(k) ?? 0) + 1);
	let hits = 0;
	for (const k of lookups) if (counts.has(k)) hits++;
	return `${counts.size}:${hits}`;
}

// A plain object has no size: Object.keys(...).length counts the keys, inside the timed region.
function countObject({ inserts, lookups }) {
	const counts = {};
	for (const k of inserts) counts[k] = (counts[k] ?? 0) + 1;
	let hits = 0;
	for (const k of lookups) if (Object.hasOwn(counts, k)) hits++;
	return `${Object.keys(counts).length}:${hits}`;
}
