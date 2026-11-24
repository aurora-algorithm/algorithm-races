// Race: lru (js)
// Extracted from suite/js/algo.mjs — build and run via suite/run.py

import { h31, XorShift32 } from "./gen.mjs";

function lruKeys(n) {
	const capacity = Math.max(1, Math.floor(n / 100));
	const keys = new XorShift32().ints(n).map((k) => k % (2 * capacity));
	return { capacity, keys };
}

// A Map finds the node and a doubly linked list keeps the order, newest at the head.
class LruCache {
	nodes = new Map();
	head = null;
	tail = null;

	unlink(node) {
		if (node.prev === null) this.head = node.next;
		else node.prev.next = node.next;
		if (node.next === null) this.tail = node.prev;
		else node.next.prev = node.prev;
	}

	pushFront(node) {
		node.prev = null;
		node.next = this.head;
		if (this.head === null) this.tail = node;
		else this.head.prev = node;
		this.head = node;
	}

	*keysOldestFirst() {
		for (let node = this.tail; node !== null; node = node.prev) yield node.key;
	}
}

function lru({ capacity, keys }) {
	const cache = new LruCache();
	let hits = 0;
	for (let i = 0; i < keys.length; i++) {
		const k = keys[i];
		const found = cache.nodes.get(k);
		if (found !== undefined) {
			cache.unlink(found);
			cache.pushFront(found);
			hits++;
			continue;
		}
		if (cache.nodes.size === capacity) {
			const oldest = cache.tail;
			cache.unlink(oldest);
			cache.nodes.delete(oldest.key);
		}
		const node = { key: k, value: i, prev: null, next: null };
		cache.nodes.set(k, node);
		cache.pushFront(node);
	}
	return `${hits}:${keys.length - hits}:${h31(cache.keysOldestFirst())}`;
}
