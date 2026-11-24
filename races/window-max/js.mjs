// Race: window-max (js)
// Extracted from suite/js/puzzle.mjs — build and run via suite/run.py

const WINDOWS = [10, 100, 1000, 10000, 100000];

function windowMax(values) {
	const n = values.length;
	const dq = new Int32Array(n);
	const hashes = [];
	for (const w of WINDOWS) {
		let head = 0;
		let tail = 0;
		let h = 0;
		for (let i = 0; i < n; i++) {
			const v = values[i];
			while (tail > head && values[dq[tail - 1]] <= v) tail--;
			dq[tail++] = i;
			if (dq[head] <= i - w) head++;
			if (i >= w - 1) h = (Math.imul(h, 31) + values[dq[head]]) >>> 0;
		}
		hashes.push(h);
	}
	return `${n}:${hashes.join(":")}`;
}
