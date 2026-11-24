// The "work" kernels (kernels/SPEC.md section 11): image resize, compression, password hashing, CSV, word count.
// main.mjs imports this file only for these kernels, so the others run without node_modules.

import { createHash } from "node:crypto";
import { createReadStream, readFileSync } from "node:fs";
import { join } from "node:path";
import { createInterface } from "node:readline";
import zlib from "node:zlib";
import argon2 from "argon2";
import bcrypt from "bcrypt";
import bcryptjs from "bcryptjs";
import { parse } from "csv-parse";
import { Jimp, ResizeStrategy } from "jimp";
import Papa from "papaparse";
import sharp from "sharp";

const PHOTOS = 8;
const TARGET_WIDTH = 1080;
const JPEG_QUALITY = 80;
const BCRYPT_COST = 10;
const ARGON2 = {
	type: argon2.argon2id,
	memoryCost: 19456,
	timeCost: 2,
	parallelism: 1,
	hashLength: 32,
};
const ARGON2_PREFIX = "$argon2id$v=19$m=19456,t=2,p=1$";
const REF_PASSWORD = "correct horse battery staple";
const BCRYPT_REF =
	"$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy";
const ARGON2_REF =
	"$argon2id$v=19$m=19456,t=2,p=1$c3RhY2tzLWJlbmNoLXNhbHQ$oLeQRnXjHhLMJZcTdYDeVr93DW7srzPjTWhPG3BTryk";

// libvips caches operations by their arguments; a warm repeat on the same buffer would be a cache hit.
sharp.cache(false);

const photos = (n, dir) =>
	Array.from({ length: n }, (_, i) =>
		readFileSync(join(dir, "photos", `photo-${i % PHOTOS}.jpg`)),
	);

async function imageSharp(images) {
	let info;
	let outBytes = 0;
	for (const data of images) {
		const out = await sharp(data)
			.resize(TARGET_WIDTH)
			// sharp alone turns on Huffman optimisation by default; pyvips and NetVips call the same libvips without it.
			.jpeg({ quality: JPEG_QUALITY, optimiseCoding: false })
			.toBuffer({ resolveWithObject: true });
		info = out.info;
		outBytes += out.data.length;
	}
	return {
		count: images.length,
		width: info.width,
		height: info.height,
		outBytes,
	};
}

async function imageJimp(images) {
	let small;
	let outBytes = 0;
	for (const data of images) {
		small = await Jimp.fromBuffer(data);
		small.resize({ w: TARGET_WIDTH, mode: ResizeStrategy.BICUBIC });
		const out = await small.getBuffer("image/jpeg", { quality: JPEG_QUALITY });
		outBytes += out.length;
	}
	return {
		count: images.length,
		width: small.width,
		height: small.height,
		outBytes,
	};
}

const imageChecksum = (r) => ({
	checksum: `${r.count}:${r.width}x${r.height}`,
	extra: { out_bytes: r.outBytes },
});

const corpus = (n, dir) => readFileSync(join(dir, `corpus-${n}.bin`));

const plainChecksum = (raw) =>
	`${raw.length}:${createHash("sha256").update(raw).digest("hex")}`;

const gzip = (raw) => zlib.gzipSync(raw, { level: 6 });
const zstd = (raw) =>
	zlib.zstdCompressSync(raw, {
		params: { [zlib.constants.ZSTD_c_compressionLevel]: 3 },
	});

const compressed = (decompress) => (packed) => ({
	checksum: plainChecksum(decompress(packed)),
	extra: { compressed_bytes: packed.length },
});

const passwords = (n) =>
	Array.from({ length: n }, (_, i) => `user-${i}-password`);

const bcryptChecksum = (lib) => (hashes) => {
	const ok = hashes.filter(
		(h, i) =>
			lib.getRounds(h) === BCRYPT_COST &&
			lib.compareSync(`user-${i}-password`, h),
	).length;
	const ref = lib.compareSync(REF_PASSWORD, BCRYPT_REF) ? 1 : 0;
	return `${hashes.length}:${ok}:${ref}`;
};

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

function addTotal(totals, customer, cents) {
	totals.set(customer, (totals.get(customer) ?? 0) + cents);
}

async function csvParse(path) {
	const totals = new Map();
	let rows = 0;
	const records = createReadStream(path).pipe(parse({ columns: true }));
	for await (const order of records) {
		rows++;
		addTotal(totals, Number(order.customer_id), Number(order.total_cents));
	}
	return { rows, totals };
}

function csvPapa(path) {
	const totals = new Map();
	let rows = 0;
	return new Promise((resolve, reject) => {
		Papa.parse(createReadStream(path), {
			header: true,
			skipEmptyLines: true,
			step: ({ data }) => {
				rows++;
				addTotal(totals, Number(data.customer_id), Number(data.total_cents));
			},
			complete: () => resolve({ rows, totals }),
			error: reject,
		});
	});
}

function top(map, compareKeys) {
	let best;
	for (const entry of map) {
		if (
			best === undefined ||
			entry[1] > best[1] ||
			(entry[1] === best[1] && compareKeys(entry[0], best[0]) < 0)
		)
			best = entry;
	}
	return best;
}

function csvChecksum({ rows, totals }) {
	let sum = 0;
	for (const v of totals.values()) sum += v;
	const [id, total] = top(totals, (a, b) => a - b);
	return `${rows}:${totals.size}:${sum}:${id}:${total}`;
}

async function wordcount(path) {
	const counts = new Map();
	const lines = createInterface({
		input: createReadStream(path),
		crlfDelay: Number.POSITIVE_INFINITY,
	});
	for await (const line of lines) {
		for (const word of line.split(" ")) {
			if (word) counts.set(word, (counts.get(word) ?? 0) + 1);
		}
	}
	return counts;
}

function wordcountChecksum(counts) {
	let total = 0;
	for (const v of counts.values()) total += v;
	const [word, count] = top(counts, (a, b) => (a < b ? -1 : a > b ? 1 : 0));
	return `${total}:${counts.size}:${word}:${count}`;
}

export const WORK_KERNELS = {
	image: {
		prepare: photos,
		run: imageSharp,
		check: imageChecksum,
		async: true,
	},
	"image-alt": {
		prepare: photos,
		run: imageJimp,
		check: imageChecksum,
		async: true,
	},
	gzip: { prepare: corpus, run: gzip, check: compressed(zlib.gunzipSync) },
	gunzip: {
		prepare: (n, dir) => gzip(corpus(n, dir)),
		run: zlib.gunzipSync,
		check: plainChecksum,
	},
	zstd: {
		prepare: corpus,
		run: zstd,
		check: compressed(zlib.zstdDecompressSync),
	},
	unzstd: {
		prepare: (n, dir) => zstd(corpus(n, dir)),
		run: zlib.zstdDecompressSync,
		check: plainChecksum,
	},
	bcrypt: {
		prepare: passwords,
		run: (pws) => pws.map((pw) => bcryptjs.hashSync(pw, BCRYPT_COST)),
		check: bcryptChecksum(bcryptjs),
	},
	"bcrypt-alt": {
		prepare: passwords,
		run: (pws) => pws.map((pw) => bcrypt.hashSync(pw, BCRYPT_COST)),
		check: bcryptChecksum(bcrypt),
	},
	argon2id: {
		prepare: passwords,
		run: argon2Hash,
		check: argon2Checksum,
		async: true,
	},
	csv: {
		prepare: (n, dir) => join(dir, `orders-${n}.csv`),
		run: csvParse,
		check: csvChecksum,
		async: true,
	},
	"csv-alt": {
		prepare: (n, dir) => join(dir, `orders-${n}.csv`),
		run: csvPapa,
		check: csvChecksum,
		async: true,
	},
	wordcount: {
		prepare: (n, dir) => join(dir, `text-${n}.txt`),
		run: wordcount,
		check: wordcountChecksum,
		async: true,
	},
};

export function libraries() {
	const names = [
		"sharp",
		"jimp",
		"bcryptjs",
		"bcrypt",
		"argon2",
		"csv-parse",
		"papaparse",
	];
	const version = (n) =>
		JSON.parse(
			readFileSync(
				join(import.meta.dirname, "node_modules", n, "package.json"),
				"utf8",
			),
		).version;
	const found = names.map((n) => `${n} ${version(n)}`);
	return `${found.join(", ")}, libvips ${sharp.versions.vips}, zlib ${process.versions.zlib ?? "?"}, zstd ${process.versions.zstd ?? "?"}`;
}
