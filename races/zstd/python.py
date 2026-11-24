# Race: zstd (python)
# Extracted from suite/python/work.py — build and run via suite/run.py

import hashlib
from pathlib import Path


def corpus(n: int, data: Path) -> bytes:
    return (data / f"corpus-{n}.bin").read_bytes()


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
