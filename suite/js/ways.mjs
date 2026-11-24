// The builtins group (kernels/builtins-NOTES.md): the language's built-in ways of doing one job, one way per
// kernel, named JOB.js.WAY (kernels/builtins/ways.json). Every way of a job gets the same input and prints the
// same checksum as python/ways.py; only the call that does the job differs.

import { fnv1a, h31, randomStrings, utf8, XorShift32 } from "./gen.mjs";

const ints = (n) => new XorShift32().ints(n);
// Doubles that fill the 52-bit mantissa: two draws per value, the high word cut to 20 bits so every value and the
// checksum sum stay exact integers below 2^53.
const ints52 = (n) => {
	const r = new XorShift32();
	return Array.from({ length: n }, () => {
		const hi = r.next() & 0xfffff;
		return hi * 4294967296 + r.next();
	});
};
const ascending = (a, b) => a - b;
const copy = (a) => a.slice();

const INT_INPUTS = {
	"bsort-int": ints,
	"bsort-int-sorted": (n) => ints(n).sort(ascending),
	"bsort-int-reversed": (n) => ints(n).sort((a, b) => b - a),
	"bsort-f52": ints52,
};

const STR_INPUTS = {
	"bsort-str": randomStrings,
	"bsort-str-sorted": (n) => randomStrings(n).sort(),
	"bsort-str-reversed": (n) => randomStrings(n).sort().reverse(),
};

const intsChecksum = (a) => String(h31(a));
const bigIntsChecksum = (a) => String(h31(Array.from(a, Number)));
const stringsChecksum = (a) =>
	String(fnv1a(utf8(a.map((s) => `${s}\n`).join(""))));

function joinedChecksum(s) {
	const bytes = utf8(s);
	return `${bytes.length}:${fnv1a(bytes)}`;
}

const INT_SORTS = {
	"Array.sort": { run: (a) => a.sort(ascending) },
	"Array.toSorted": { run: (a) => a.toSorted(ascending) },
	"Float64Array.sort": {
		convert: (a) => Float64Array.from(a),
		run: (a) => a.sort(),
	},
	"BigInt64Array.sort": {
		convert: (a) => BigInt64Array.from(a, BigInt),
		run: (a) => a.sort(),
		check: bigIntsChecksum,
	},
};

const STR_SORTS = {
	"Array.sort": (a) => a.sort(),
	"Array.toSorted": (a) => a.toSorted(),
};

function mapIntInput(n) {
	const inserts = ints(n);
	return { inserts, lookups: inserts.map((k, i) => (k ^ (i & 1)) >>> 0) };
}

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

// Keeps the flattening read from being dropped as dead code.
export let flatProbe = 0;

// += builds a rope (V8 ConsString, JSC JSRopeString); the first character read turns it into one flat string, so
// the timed result is a finished string like Array.join's, not a tree the checksum would flatten after the timer.
function joinPlusEquals(parts) {
	let s = "";
	for (let i = 0; i < parts.length; i++) {
		if (i > 0) s += ",";
		s += parts[i];
	}
	flatProbe = s.charCodeAt(0);
	return s;
}

const MAPS = { Map: countMap, Object: countObject };
const JOINS = {
	"Array.join": (parts) => parts.join(","),
	"+=": joinPlusEquals,
};

function buildTable() {
	const table = {};
	for (const [job, make] of Object.entries(INT_INPUTS)) {
		for (const [way, w] of Object.entries(INT_SORTS)) {
			const convert = w.convert ?? ((a) => a);
			table[`${job}.js.${way}`] = {
				prepare: (n) => convert(make(n)),
				fresh: copy,
				run: w.run,
				check: w.check ?? intsChecksum,
			};
		}
	}
	for (const [job, make] of Object.entries(STR_INPUTS)) {
		for (const [way, run] of Object.entries(STR_SORTS)) {
			table[`${job}.js.${way}`] = {
				prepare: make,
				fresh: copy,
				run,
				check: stringsChecksum,
			};
		}
	}
	for (const [way, run] of Object.entries(MAPS)) {
		table[`bmap-int.js.${way}`] = { prepare: mapIntInput, run, check: String };
		table[`bmap-str.js.${way}`] = {
			prepare: mapStrInput,
			fresh: freshStrings,
			run,
			check: String,
		};
	}
	for (const [way, run] of Object.entries(JOINS)) {
		table[`bjoin.js.${way}`] = {
			prepare: (n) => ints(n).map(String),
			run,
			check: joinedChecksum,
		};
	}
	return table;
}

export const WAYS_KERNELS = buildTable();
