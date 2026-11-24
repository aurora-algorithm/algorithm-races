# Race: gzip (python)
# Extracted from suite/python/work.py — build and run via suite/run.py

import gzip
import hashlib
from pathlib import Path


def corpus(n: int, data: Path) -> bytes:
    return (data / f"corpus-{n}.bin").read_bytes()


def gzip_compress(raw: bytes) -> bytes:
    return gzip.compress(raw, compresslevel=6)


def compressed_checksum(decompress):
    def check(packed: bytes) -> tuple[str, dict]:
        raw = decompress(packed)
        return f"{len(raw)}:{hashlib.sha256(raw).hexdigest()}", {"compressed_bytes": len(packed)}

    return check
