// Race: heapsort (js)
// Extracted from suite/js/sorts.mjs — build and run via suite/run.py

function swap(a, i, j) {
	const t = a[i];
	a[i] = a[j];
	a[j] = t;
}

function siftDown(a, root, end) {
	while (2 * root + 1 < end) {
		let child = 2 * root + 1;
		if (child + 1 < end && a[child] < a[child + 1]) child++;
		if (a[root] < a[child]) {
			swap(a, root, child);
			root = child;
		} else {
			return;
		}
	}
}

export function heapsort(a) {
	const n = a.length;
	for (let start = (n >> 1) - 1; start >= 0; start--) siftDown(a, start, n);
	for (let end = n - 1; end > 0; end--) {
		swap(a, 0, end);
		siftDown(a, 0, end);
	}
	return a;
}
