// Race: image (js)
// Extracted from suite/js/work.mjs — build and run via suite/run.py

import { createReadStream, readFileSync } from "node:fs";
import { join } from "node:path";
import sharp from "sharp";

const PHOTOS = 8;

const TARGET_WIDTH = 1080;

const JPEG_QUALITY = 80;

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

const imageChecksum = (r) => ({
	checksum: `${r.count}:${r.width}x${r.height}`,
	extra: { out_bytes: r.outBytes },
});
