// Race: sort-int (js)
// Extracted from suite/js/main.mjs — build and run via suite/run.py

export function sortInt(a) {
	a.sort((x, y) => x - y);
	return a;
}
