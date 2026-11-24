# Race: bjoin.go.strings.Join (python)
# Extracted from suite/python/ways.py — build and run via suite/run.py

from gen import XorShift32, fnv1a, h31, random_strings


def ints(n: int) -> list[int]:
    return XorShift32().ints(n)


def join_parts(n: int) -> list[str]:
    return [str(v) for v in ints(n)]


def joined_checksum(s: str) -> str:
    data = s.encode()
    return f"{len(data)}:{fnv1a(data)}"


def join_str_join(parts: list[str]) -> str:
    return ",".join(parts)


def join_plus_equals(parts: list[str]) -> str:
    out = ""
    for i, part in enumerate(parts):
        if i:
            out += ","
        out += part
    return out
