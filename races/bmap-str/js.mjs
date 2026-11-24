// Race: bmap-str.go.map (js)
// Extracted from suite/js/ways.mjs — build and run via suite/run.py

import { fnv1a, h31, randomStrings, utf8, XorShift32 } from "./gen.mjs";

// The keys as char codes in one buffer: fresh() rebuilds them as new strings for every repeat.
function packStrings(strings) {
	const ends = new Int32Array(strings.length);
	let total = 0;
	for (let i = 0; i < strings.length; i++) {
		total += strings[i].length;
		ends[i] = total;
	}
	const codes = new Uint8Array(total);
	let at = 0;
	for (const s of strings) {
		for (let c = 0; c < s.length; c++) codes[at++] = s.charCodeAt(c);
	}
	return { codes, ends };
}

// String.fromCharCode makes a new flat (sequential) string with no hash computed yet, in V8 and JSC alike.
function unpackStrings({ codes, ends }) {
	const out = new Array(ends.length);
	let start = 0;
	for (let i = 0; i < ends.length; i++) {
		out[i] = String.fromCharCode.apply(null, codes.subarray(start, ends[i]));
		start = ends[i];
	}
	return out;
}

function mapStrInput(n) {
	const inserts = randomStrings(n);
	return {
		inserts: packStrings(inserts),
		lookups: packStrings(
			inserts.map((k, i) => (i % 2 === 0 ? k : k.slice(0, -1))),
		),
	};
}

// Every repeat gets new string objects with the same text, for the inserts and separately for the lookups (made
// untimed): V8 and JSC cache a string's hash in the string, so reused strings would skip hashing from repeat 2 on.
const freshStrings = ({ inserts, lookups }) => ({
	inserts: unpackStrings(inserts),
	lookups: unpackStrings(lookups),
});

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
