// Race: bsort-str-reversed.rust.sort (js)
// Extracted from suite/js/ways.mjs — build and run via suite/run.py

import { fnv1a, h31, randomStrings, utf8, XorShift32 } from "./gen.mjs";

const STR_INPUTS = {
	"bsort-str": randomStrings,
	"bsort-str-sorted": (n) => randomStrings(n).sort(),
	"bsort-str-reversed": (n) => randomStrings(n).sort().reverse(),
};

const STR_SORTS = {
	"Array.sort": (a) => a.sort(),
	"Array.toSorted": (a) => a.toSorted(),
};
