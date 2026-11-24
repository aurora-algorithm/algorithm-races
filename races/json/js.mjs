// Race: json (js)
// Extracted from suite/js/misc.mjs — build and run via suite/run.py

export function jsonRoundtrip(text) {
	const doc = JSON.parse(text);
	let total = 0;
	let items = 0;
	for (const order of doc.orders) {
		total += order.total_cents;
		items += order.items;
	}
	return { count: doc.orders.length, total, items, out: JSON.stringify(doc) };
}
