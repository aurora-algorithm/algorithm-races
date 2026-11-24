// Race: quicksort (js)
// Extracted from suite/js/sorts.mjs — build and run via suite/run.py

function swap(a, i, j) {
	const t = a[i];
	a[i] = a[j];
	a[j] = t;
}

export function quicksort(a, lo, hi) {
	while (lo < hi) {
		const pivot = a[lo + ((hi - lo) >> 1)];
		let i = lo - 1;
		let j = hi + 1;
		while (true) {
			do {
				i++;
			} while (a[i] < pivot);
			do {
				j--;
			} while (a[j] > pivot);
			if (i >= j) break;
			swap(a, i, j);
		}
		if (j - lo < hi - j) {
			quicksort(a, lo, j);
			lo = j + 1;
		} else {
			quicksort(a, j + 1, hi);
			hi = j;
		}
	}
}

export function runQuicksort(a) {
	quicksort(a, 0, a.length - 1);
	return a;
}
