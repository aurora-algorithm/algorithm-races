// Race: bcrypt (js)
// Extracted from suite/js/work.mjs — build and run via suite/run.py

const BCRYPT_COST = 10;

const REF_PASSWORD = "correct horse battery staple";

const BCRYPT_REF =
	"$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy";

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
