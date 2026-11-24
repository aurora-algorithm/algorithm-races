// Race: argon2id (js)
// Extracted from suite/js/work.mjs — build and run via suite/run.py

import argon2 from "argon2";

const ARGON2 = {
	type: argon2.argon2id,
	memoryCost: 19456,
	timeCost: 2,
	parallelism: 1,
	hashLength: 32,
};

const ARGON2_PREFIX = "$argon2id$v=19$m=19456,t=2,p=1$";

const REF_PASSWORD = "correct horse battery staple";

const ARGON2_REF =
	"$argon2id$v=19$m=19456,t=2,p=1$c3RhY2tzLWJlbmNoLXNhbHQ$oLeQRnXjHhLMJZcTdYDeVr93DW7srzPjTWhPG3BTryk";

const passwords = (n) =>
	Array.from({ length: n }, (_, i) => `user-${i}-password`);

async function argon2Hash(pws) {
	const out = [];
	for (const pw of pws) out.push(await argon2.hash(pw, ARGON2));
	return out;
}

async function argon2Checksum(hashes) {
	let ok = 0;
	for (const [i, h] of hashes.entries()) {
		if (
			h.startsWith(ARGON2_PREFIX) &&
			(await argon2.verify(h, `user-${i}-password`))
		)
			ok++;
	}
	const ref = (await argon2.verify(ARGON2_REF, REF_PASSWORD)) ? 1 : 0;
	return `${hashes.length}:${ok}:${ref}`;
}
