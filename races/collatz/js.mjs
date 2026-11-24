// Race: collatz (js)
// Extracted from suite/js/puzzle.mjs — build and run via suite/run.py

function collatz(n) {
	const cache = new Int32Array(n);
	cache[1] = 1;
	let best = 1;
	let bestLength = 1;
	let total = 1;
	for (let start = 2; start < n; start++) {
		let x = start;
		let steps = 0;
		while (x >= start) {
			x = x % 2 === 0 ? x / 2 : 3 * x + 1;
			steps++;
		}
		const length = steps + cache[x];
		cache[start] = length;
		total += length;
		if (length > bestLength) {
			best = start;
			bestLength = length;
		}
	}
	return `${n}:${best}:${bestLength}:${total}`;
}
