// Sorting kernels, written the same way in every language (kernels/SPEC.md section 3).

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

export function radixSort(a) {
	let src = a;
	let dst = new Array(a.length).fill(0);
	for (const shift of [0, 8, 16, 24]) {
		const count = new Array(256).fill(0);
		for (const v of src) count[(v >>> shift) & 255]++;
		const pos = new Array(256).fill(0);
		let total = 0;
		for (let d = 0; d < 256; d++) {
			pos[d] = total;
			total += count[d];
		}
		for (const v of src) dst[pos[(v >>> shift) & 255]++] = v;
		[src, dst] = [dst, src];
	}
	return src;
}

export function runQuicksort(a) {
	quicksort(a, 0, a.length - 1);
	return a;
}

export function runMergesort(a) {
	mergeSort(a, new Array(a.length).fill(0), 0, a.length);
	return a;
}
