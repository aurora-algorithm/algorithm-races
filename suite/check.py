#!/usr/bin/env python3
"""Runs every kernel at its quick size with one program and compares the checksums with expected.json.

usage: python3 kernels/check.py [--kernel K ...] [--group G ...] [--lang L] [--cwd DIR] -- PROGRAM [ARGS...]
e.g.   python3 kernels/check.py --cwd kernels/rust --lang rust -- target/release/kernels
--lang skips the library-swap kernels that language does not have (kernels.json "langs").
"""

import argparse
import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from harness import data  # noqa: E402


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--kernel", action="append")
    ap.add_argument("--group", action="append")
    ap.add_argument("--lang")
    ap.add_argument("--cwd", default=".")
    ap.add_argument("program", nargs="+")
    args = ap.parse_args()
    table = json.loads((HERE / "kernels.json").read_text())
    expected = json.loads((HERE / "expected.json").read_text())
    bad = 0
    for name, k in table.items():
        if (args.kernel and name not in args.kernel) or (args.group and k["group"] not in args.group):
            continue
        if args.lang and "langs" in k and args.lang not in k["langs"]:
            continue
        data.ensure(name, k, k["quick"])
        for threads in k.get("threads", [1]):
            argv = [*args.program, name, "--size", str(k["quick"]), "--repeats", "2", "--threads", str(threads)]
            argv += ["--data", str(data.DATA)]
            r = subprocess.run(argv, cwd=args.cwd, capture_output=True, text=True)
            want = expected.get(f"{name}@{k['quick']}")
            try:
                got = json.loads(r.stdout.strip().splitlines()[-1])["checksum"]
            except (IndexError, ValueError, KeyError):
                got = f"no JSON (exit {r.returncode}): {r.stderr.strip()[-300:]}"
            ok = got == want
            bad += not ok
            print(f"{'ok ' if ok else 'BAD'} {name:16} T={threads} {got}" + ("" if ok else f"  want {want}"))
    print("all match" if not bad else f"{bad} mismatches")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
