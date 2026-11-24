// TextEncoder / TextDecoder for engines that lack them (QuickJS-ng's qjs and standalone GraalJS have neither;
// txiki.js has no latin1 decoder). The kernels only encode and decode ASCII text, outside the timed region.

const g = globalThis;

function encodeUtf8(text) {
	const out = [];
	for (const ch of text) {
		const c = ch.codePointAt(0);
		if (c < 0x80) out.push(c);
		else if (c < 0x800) out.push(0xc0 | (c >> 6), 0x80 | (c & 63));
		else if (c < 0x10000)
			out.push(0xe0 | (c >> 12), 0x80 | ((c >> 6) & 63), 0x80 | (c & 63));
		else
			out.push(
				0xf0 | (c >> 18),
				0x80 | ((c >> 12) & 63),
				0x80 | ((c >> 6) & 63),
				0x80 | (c & 63),
			);
	}
	return Uint8Array.from(out);
}

function decodeLatin1(bytes) {
	let out = "";
	for (let i = 0; i < bytes.length; i += 8192) {
		out += String.fromCharCode(...bytes.subarray(i, i + 8192));
	}
	return out;
}

function hasLatin1() {
	try {
		return new g.TextDecoder("latin1").decode(new Uint8Array([65])) === "A";
	} catch {
		return false;
	}
}

if (typeof g.TextEncoder !== "function") {
	g.TextEncoder = class {
		encode(text = "") {
			return encodeUtf8(String(text));
		}
	};
}

if (typeof g.TextDecoder !== "function" || !hasLatin1()) {
	const Native = g.TextDecoder;
	g.TextDecoder = class {
		constructor(label = "utf-8") {
			this.latin1 = ["latin1", "iso-8859-1", "ascii"].includes(
				label.toLowerCase(),
			);
			this.native = !this.latin1 && Native ? new Native(label) : null;
		}
		decode(bytes) {
			if (this.native) return this.native.decode(bytes);
			if (this.latin1) return decodeLatin1(bytes);
			throw new Error("utf-8 decoding is not polyfilled");
		}
	};
}
