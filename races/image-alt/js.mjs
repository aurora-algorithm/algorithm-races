// Race: image-alt (js)
// Extracted from suite/js/work.mjs — build and run via suite/run.py

import { createReadStream, readFileSync } from "node:fs";
import { join } from "node:path";
import { Jimp, ResizeStrategy } from "jimp";

const PHOTOS = 8;

const TARGET_WIDTH = 1080;

const JPEG_QUALITY = 80;

const photos = (n, dir) =>
	Array.from({ length: n }, (_, i) =>
		readFileSync(join(dir, "photos", `photo-${i % PHOTOS}.jpg`)),
	);

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
