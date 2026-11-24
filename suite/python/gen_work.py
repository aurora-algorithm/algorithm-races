"""Input files of the "work" kernels (kernels/SPEC.md section 11). Standard library only; the photos need Pillow and
live in photos.py. Every other language reads these files, so only this generator has to be deterministic.

usage: python gen_work.py KIND SIZE DIR     KIND = corpus | text | csv
"""

import sys
from pathlib import Path

from gen import METHODS, ORDER_STATUS, STATUSES, XorShift32

MIB = 1 << 20
VOCAB = 20_000
IDENTIFIERS = 256


def _word(rng: XorShift32, min_len: int, spread: int) -> str:
    return "".join(chr(97 + rng.next() % 26) for _ in range(min_len + rng.next() % spread))


def _log_lines(rng: XorShift32, lines: int) -> str:
    out = []
    for _ in range(lines):
        method = METHODS[rng.next() % 4]
        version = 1 + rng.next() % 3
        user = rng.next() % 100000
        status = STATUSES[rng.next() % 12]
        ms = rng.next() % 1000
        out.append(
            f"2026-11-13T09:{ms % 60:02d}:{user % 60:02d}Z {method} /api/v{version}/users/{user} {status} {ms}ms\n"
        )
    return "".join(out)


def _order_lines(first: int, count: int) -> str:
    out = []
    for i in range(first, first + count):
        out.append(
            f'{{"id":{i},"customer_id":{(i - 1) // 20 + 1},"status":"{ORDER_STATUS[i % 4]}",'
            f'"total_cents":{500 + (i * 7919) % 50000},"items":{1 + (i * 31) % 9},'
            f'"tags":["a{i % 7}","b{i % 11}"]}}\n'
        )
    return "".join(out)


def _source_chunk(rng: XorShift32, ids: list[str], functions: int) -> str:
    out = []
    for _ in range(functions):
        a, b, c, d, e, f, g = (ids[rng.next() % IDENTIFIERS] for _ in range(7))
        n = rng.next() % 1000
        out.append(
            f"def {a}_{b}(self, {c}, {d}=None):\n"
            f'    """Return the {e} for {c}."""\n'
            f"    if {d} is None:\n"
            f"        {d} = self.{f}.get({c}, {n})\n"
            f"    for item in self.{g}:\n"
            f"        if item.{e} > {d}:\n"
            f"            return item\n"
            f"    return None\n\n\n"
        )
    return "".join(out)


def corpus(size: int) -> bytes:
    """Rounds of 400 log lines, 100 JSON order lines and 10 Python-like functions, cut at `size` bytes."""
    rng = XorShift32()
    ids = [_word(rng, 3, 8) for _ in range(IDENTIFIERS)]
    parts, total, order = [], 0, 1
    while total < size:
        chunk = (_log_lines(rng, 400) + _order_lines(order, 100) + _source_chunk(rng, ids, 10)).encode()
        order += 100
        parts.append(chunk)
        total += len(chunk)
    return b"".join(parts)[:size]


def text_block() -> bytes:
    """Exactly 1 MiB: lines of 1..15 words joined by single spaces, padded with newlines."""
    rng = XorShift32()
    vocab = [_word(rng, 2, 9) for _ in range(VOCAB)]
    lines, total = [], 0
    while True:
        count = 1 + rng.next() % 15
        words = []
        for _ in range(count):
            a = rng.next() % VOCAB
            b = rng.next() % VOCAB
            words.append(vocab[a * b // VOCAB])
        line = " ".join(words) + "\n"
        if total + len(line) > MIB:
            break
        lines.append(line)
        total += len(line)
    return ("".join(lines) + "\n" * (MIB - total)).encode()


CSV_HEADER = "id,customer_id,status,total_cents,items,note\n"
CUSTOMERS = 100_000


def csv_note(i: int) -> str:
    if i % 10 == 0:
        return '"Leave at the door, ring twice"'
    if i % 10 == 1:
        return '"Box marked ""fragile"""'
    return "standard"


def csv_rows(first: int, last: int) -> str:
    return "".join(
        f"{i},{1 + (i * 48271) % CUSTOMERS},{ORDER_STATUS[i % 4]},{500 + (i * 7919) % 50000},"
        f"{1 + (i * 31) % 9},{csv_note(i)}\n"
        for i in range(first, last + 1)
    )


def write_csv(rows: int, path: Path) -> None:
    with path.open("w", encoding="ascii", newline="") as f:
        f.write(CSV_HEADER)
        step = 100_000
        for first in range(1, rows + 1, step):
            f.write(csv_rows(first, min(first + step - 1, rows)))


def write(kind: str, size: int, data: Path) -> Path:
    data.mkdir(parents=True, exist_ok=True)
    path = data / {"corpus": f"corpus-{size}.bin", "text": f"text-{size}.txt", "csv": f"orders-{size}.csv"}[kind]
    if path.exists():
        return path
    tmp = path.with_name(path.name + ".tmp")
    if kind == "csv":
        write_csv(size, tmp)
    elif kind == "text":
        if size % MIB:
            raise ValueError("text size must be a multiple of 1 MiB")
        block = text_block()
        with tmp.open("wb") as f:
            for _ in range(size // MIB):
                f.write(block)
    else:
        tmp.write_bytes(corpus(size))
    tmp.replace(path)
    return path


if __name__ == "__main__":
    print(write(sys.argv[1], int(sys.argv[2]), Path(sys.argv[3])))
