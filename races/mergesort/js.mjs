// Race: mergesort (js)
// Extracted from suite/js/sorts.mjs — build and run via suite/run.py

export function mergeSort(a, aux, lo, hi) {
	if (hi - lo < 2) return;
	const mid = lo + ((hi - lo) >> 1);
	mergeSort(a, aux, lo, mid);
	mergeSort(a, aux, mid, hi);
	merge(a, aux, lo, mid, hi);
}

export function merge(a, aux, lo, mid, hi) {
	for (let k = lo; k < hi; k++) aux[k] = a[k];
	let i = lo;
	let j = mid;
	let k = lo;
	while (i < mid && j < hi) {
		if (aux[j] < aux[i]) a[k++] = aux[j++];
		else a[k++] = aux[i++];
	}
	while (i < mid) a[k++] = aux[i++];
	while (j < hi) a[k++] = aux[j++];
}

export function runMergesort(a) {
	mergeSort(a, new Array(a.length).fill(0), 0, a.length);
	return a;
}
