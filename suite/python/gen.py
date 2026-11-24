"""Shared inputs and checksums (kernels/SPEC.md section 2). Every language implements exactly these."""

SEED = 2463534242
MASK = 0xFFFFFFFF
FNV_OFFSET = 2166136261
FNV_PRIME = 16777619


class XorShift32:
    __slots__ = ("s",)

    def __init__(self, seed: int = SEED) -> None:
        self.s = seed

    def next(self) -> int:
        s = self.s
        s ^= (s << 13) & MASK
        s ^= s >> 17
        s ^= (s << 5) & MASK
        self.s = s
        return s

    def ints(self, n: int) -> list[int]:
        s = self.s
        out = [0] * n
        for i in range(n):
            s ^= (s << 13) & MASK
            s ^= s >> 17
            s ^= (s << 5) & MASK
            out[i] = s
        self.s = s
        return out


def h31(values) -> int:
    h = 0
    for v in values:
        h = (h * 31 + int(v)) & MASK
    return h


def fnv1a(data: bytes | bytearray, h: int = FNV_OFFSET) -> int:
    for b in data:
        h = ((h ^ b) * FNV_PRIME) & MASK
    return h


def random_strings(n: int) -> list[str]:
    rng = XorShift32()
    out = []
    for _ in range(n):
        length = 8 + rng.next() % 9
        out.append("".join(chr(97 + rng.next() % 26) for _ in range(length)))
    return out


def sha_buffer(size: int) -> bytes:
    block = XorShift32().ints(1 << 18)
    one_mib = b"".join(v.to_bytes(4, "little") for v in block)
    return one_mib * (size // (1 << 20))


METHODS = ("GET", "POST", "PUT", "DELETE")
STATUSES = (200, 201, 204, 301, 304, 400, 401, 403, 404, 500, 502, 503)


def log_text(lines: int) -> str:
    rng = XorShift32()
    parts = []
    for _ in range(lines):
        method = METHODS[rng.next() % 4]
        version = 1 + rng.next() % 3
        user = rng.next() % 100000
        status = STATUSES[rng.next() % 12]
        ms = rng.next() % 1000
        parts.append(f"{method} /api/v{version}/users/{user} {status} {ms}ms\n")
    return "".join(parts)


def matrices(n: int) -> tuple[list[float], list[float]]:
    rng = XorShift32()
    a = [float(v % 10) for v in rng.ints(n * n)]
    b = [float(v % 10) for v in rng.ints(n * n)]
    return a, b


ORDER_STATUS = ("paid", "shipped", "delivered", "refunded")


def orders_json(n: int) -> str:
    rows = []
    for i in range(1, n + 1):
        rows.append(
            f'{{"id":{i},"customer_id":{(i - 1) // 20 + 1},"status":"{ORDER_STATUS[i % 4]}",'
            f'"total_cents":{500 + (i * 7919) % 50000},"items":{1 + (i * 31) % 9},'
            f'"tags":["a{i % 7}","b{i % 11}"]}}'
        )
    return '{"orders":[' + ",".join(rows) + "]}"
