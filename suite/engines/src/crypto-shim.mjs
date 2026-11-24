// Stands in for node:crypto on engines without it (QuickJS-ng, txiki.js, standalone GraalJS): the sha256 kernel is
// listed out for those runtimes in kernels/runtimes.json, so this only fires if someone runs it anyway.

export function createHash(algorithm) {
	throw new Error(`createHash(${algorithm}) is not available on this engine`);
}
