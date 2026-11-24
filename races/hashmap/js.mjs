// Race: hashmap (js)
// Extracted from suite/js/misc.mjs — build and run via suite/run.py

export function hashmap(inserts, lookups) {
	const counts = new Map();
	for (const k of inserts) counts.set(k, (counts.get(k) ?? 0) + 1);
	let hits = 0;
	for (const k of lookups) {
		if (counts.has(k)) hits++;
	}
	return `${counts.size}:${hits}`;
}
