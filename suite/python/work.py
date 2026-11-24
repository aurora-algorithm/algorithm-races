"""The "work" kernels (kernels/SPEC.md section 11): image resize, compression, password hashing, CSV, word count.
Third-party libraries are imported inside each kernel, so the other kernels run on a bare interpreter."""

import contextlib
import gzip
import hashlib
import io
import os
from pathlib import Path

PHOTOS = 8
TARGET_WIDTH = 1080
JPEG_QUALITY = 80
BCRYPT_COST = 10
ARGON2_M, ARGON2_T, ARGON2_P = 19456, 2, 1
BCRYPT_REF = "$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy"
ARGON2_REF = "$argon2id$v=19$m=19456,t=2,p=1$c3RhY2tzLWJlbmNoLXNhbHQ$oLeQRnXjHhLMJZcTdYDeVr93DW7srzPjTWhPG3BTryk"
REF_PASSWORD = "correct horse battery staple"


def photos(n: int, data: Path) -> list[bytes]:
    return [(data / "photos" / f"photo-{i % PHOTOS}.jpg").read_bytes() for i in range(n)]


def target_height(width: int, height: int) -> int:
    return (height * TARGET_WIDTH + width // 2) // width


def image_pillow(images: list[bytes]) -> tuple[int, int, int, int]:
    from PIL import Image

    out_bytes = 0
    for data in images:
        with Image.open(io.BytesIO(data)) as img:
            small = img.resize((TARGET_WIDTH, target_height(*img.size)), Image.Resampling.LANCZOS)
        buf = io.BytesIO()
        small.save(buf, "JPEG", quality=JPEG_QUALITY)
        out_bytes += buf.tell()
    return len(images), small.width, small.height, out_bytes


def image_pyvips(images: list[bytes]) -> tuple[int, int, int, int]:
    import pyvips

    # libvips caches operations by their arguments; a warm repeat on the same bytes would be a cache hit.
    pyvips.cache_set_max(0)
    out_bytes = 0
    for data in images:
        small = pyvips.Image.thumbnail_buffer(data, TARGET_WIDTH)
        out_bytes += len(small.jpegsave_buffer(Q=JPEG_QUALITY))
    return len(images), small.width, small.height, out_bytes


def image_checksum(result: tuple[int, int, int, int]) -> tuple[str, dict]:
    count, width, height, out_bytes = result
    return f"{count}:{width}x{height}", {"out_bytes": out_bytes}


def corpus(n: int, data: Path) -> bytes:
    return (data / f"corpus-{n}.bin").read_bytes()


def gzip_compress(raw: bytes) -> bytes:
    return gzip.compress(raw, compresslevel=6)


def zstd_compress(raw: bytes) -> bytes:
    from compression import zstd

    return zstd.compress(raw, level=3)


def zstd_decompress(packed: bytes) -> bytes:
    from compression import zstd

    return zstd.decompress(packed)


def compressed_checksum(decompress):
    def check(packed: bytes) -> tuple[str, dict]:
        raw = decompress(packed)
        return f"{len(raw)}:{hashlib.sha256(raw).hexdigest()}", {"compressed_bytes": len(packed)}

    return check


def plain_checksum(raw: bytes) -> str:
    return f"{len(raw)}:{hashlib.sha256(raw).hexdigest()}"


def passwords(n: int, _data: Path) -> list[str]:
    return [f"user-{i}-password" for i in range(n)]


def bcrypt_hash(pws: list[str]) -> list[str]:
    import bcrypt

    return [bcrypt.hashpw(pw.encode(), bcrypt.gensalt(BCRYPT_COST)).decode() for pw in pws]


def bcrypt_checksum(hashes: list[str]) -> str:
    import bcrypt

    ok = sum(
        h[3:7] == f"${BCRYPT_COST}$" and bcrypt.checkpw(f"user-{i}-password".encode(), h.encode())
        for i, h in enumerate(hashes)
    )
    ref = bcrypt.checkpw(REF_PASSWORD.encode(), BCRYPT_REF.encode())
    return f"{len(hashes)}:{ok}:{int(ref)}"


def argon2_hasher():
    from argon2 import PasswordHasher, Type

    return PasswordHasher(
        time_cost=ARGON2_T, memory_cost=ARGON2_M, parallelism=ARGON2_P, hash_len=32, salt_len=16, type=Type.ID
    )


def argon2_hash(pws: list[str]) -> list[str]:
    hasher = argon2_hasher()
    return [hasher.hash(pw) for pw in pws]


def argon2_checksum(hashes: list[str]) -> str:
    from argon2.exceptions import VerifyMismatchError

    hasher = argon2_hasher()
    prefix = f"$argon2id$v=19$m={ARGON2_M},t={ARGON2_T},p={ARGON2_P}$"

    def verifies(h: str, pw: str) -> bool:
        try:
            return hasher.verify(h, pw)
        except VerifyMismatchError:
            return False

    ok = sum(h.startswith(prefix) and verifies(h, f"user-{i}-password") for i, h in enumerate(hashes))
    return f"{len(hashes)}:{ok}:{int(verifies(ARGON2_REF, REF_PASSWORD))}"


def csv_path(n: int, data: Path) -> Path:
    return data / f"orders-{n}.csv"


def csv_stdlib(path: Path) -> tuple[int, dict[int, int]]:
    import csv

    totals: dict[int, int] = {}
    rows = 0
    with path.open(newline="", encoding="utf-8") as f:
        reader = csv.reader(f)
        header = next(reader)
        cust, total = header.index("customer_id"), header.index("total_cents")
        for row in reader:
            rows += 1
            key = int(row[cust])
            totals[key] = totals.get(key, 0) + int(row[total])
    return rows, totals


def csv_polars(path: Path) -> tuple[int, dict[int, int]]:
    import polars as pl

    # One query, one pass over the file: the row count is the sum of the per-customer counts.
    sums = pl.scan_csv(path).group_by("customer_id").agg(pl.col("total_cents").sum(), pl.len().alias("rows")).collect()
    rows = int(sums["rows"].sum())
    return rows, dict(zip(sums["customer_id"].to_list(), sums["total_cents"].to_list(), strict=True))


def csv_checksum(result: tuple[int, dict[int, int]]) -> str:
    rows, totals = result
    top = min(totals.items(), key=lambda kv: (-kv[1], kv[0]))
    return f"{rows}:{len(totals)}:{sum(totals.values())}:{top[0]}:{top[1]}"


def text_path(n: int, data: Path) -> Path:
    return data / f"text-{n}.txt"


def wordcount(path: Path) -> dict[str, int]:
    counts: dict[str, int] = {}
    with path.open(encoding="utf-8") as f:
        for line in f:
            for word in line.split():
                counts[word] = counts.get(word, 0) + 1
    return counts


def wordcount_checksum(counts: dict[str, int]) -> str:
    top = min(counts.items(), key=lambda kv: (-kv[1], kv[0]))
    return f"{sum(counts.values())}:{len(counts)}:{top[0]}:{top[1]}"


def single(fn):
    return lambda item, _threads, _pool: fn(item)


def kernels(threads: int) -> dict:
    os.environ.setdefault("POLARS_MAX_THREADS", str(threads))
    table = {
        "image": {"prepare": photos, "run": image_pillow, "check": image_checksum},
        "image-alt": {"prepare": photos, "run": image_pyvips, "check": image_checksum},
        "gzip": {"prepare": corpus, "run": gzip_compress, "check": compressed_checksum(gzip.decompress)},
        "gunzip": {
            "prepare": lambda n, d: gzip_compress(corpus(n, d)),
            "run": gzip.decompress,
            "check": plain_checksum,
        },
        "zstd": {"prepare": corpus, "run": zstd_compress, "check": compressed_checksum(zstd_decompress)},
        "unzstd": {
            "prepare": lambda n, d: zstd_compress(corpus(n, d)),
            "run": zstd_decompress,
            "check": plain_checksum,
        },
        "bcrypt": {"prepare": passwords, "run": bcrypt_hash, "check": bcrypt_checksum},
        "argon2id": {"prepare": passwords, "run": argon2_hash, "check": argon2_checksum},
        "csv": {"prepare": csv_path, "run": csv_stdlib, "check": csv_checksum},
        "csv-alt": {"prepare": csv_path, "run": csv_polars, "check": csv_checksum},
        "wordcount": {"prepare": text_path, "run": wordcount, "check": wordcount_checksum},
    }
    return {name: k | {"run": single(k["run"])} for name, k in table.items()}


def libraries() -> str:
    from importlib.metadata import PackageNotFoundError, version

    found = []
    for name in ("pillow", "pyvips", "pyvips-binary", "bcrypt", "argon2-cffi", "polars"):
        with contextlib.suppress(PackageNotFoundError):
            found.append(f"{name} {version(name)}")
    return ", ".join(found)
