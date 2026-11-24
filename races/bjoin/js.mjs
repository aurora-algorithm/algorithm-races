// Race: bjoin.go.strings.Join (js)
// Extracted from suite/js/ways.mjs — build and run via suite/run.py

import { fnv1a, h31, randomStrings, utf8, XorShift32 } from "./gen.mjs";

function joinedChecksum(s) {
	const bytes = utf8(s);
	return `${bytes.length}:${fnv1a(bytes)}`;
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

const JOINS = {
	"Array.join": (parts) => parts.join(","),
	"+=": joinPlusEquals,
};
