// Race: bsort-int-sorted.goose.sort (js)
// Extracted from suite/js/ways.mjs — build and run via suite/run.py

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

const INT_INPUTS = {
	"bsort-int": ints,
	"bsort-int-sorted": (n) => ints(n).sort(ascending),
	"bsort-int-reversed": (n) => ints(n).sort((a, b) => b - a),
	"bsort-f52": ints52,
};

const bigIntsChecksum = (a) => String(h31(Array.from(a, Number)));

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
