# Race: unzstd (python)
# Extracted from suite/python/work.py — build and run via suite/run.py

import hashlib


def zstd_compress(raw: bytes) -> bytes:
    from compression import zstd

    return zstd.compress(raw, level=3)


def zstd_decompress(packed: bytes) -> bytes:
    from compression import zstd

    return zstd.decompress(packed)


def plain_checksum(raw: bytes) -> str:
    return f"{len(raw)}:{hashlib.sha256(raw).hexdigest()}"
