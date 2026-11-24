"""Screen capture of one kruskal run (Short 163's hook): prints each step live with a timestamp, never a whole-process
race. Builds the edges, then runs the kruskal cell's timed section (sort, then union-find) and prints the build, sort
and loop split and the checksum against expected.json.

usage (from kernels/python):  python kruskal_capture.py --size N [--warm 1]
  --warm W   untimed full runs first (each on a fresh copy), so the captured run is warm like the measured cells
"""

import argparse
import json
import platform
import sys
import time
from operator import itemgetter
from pathlib import Path

import algo

EXPECTED = Path(__file__).resolve().parent.parent / "expected.json"


def say(start: int, text: str) -> None:
    print(f"[{(time.perf_counter_ns() - start) / 1e9:8.3f} s] {text}", flush=True)


def timed_section(n: int, edges: list[tuple[int, int, int]]) -> tuple[str, float, float]:
    t0 = time.perf_counter_ns()
    edges.sort(key=itemgetter(0))
    t1 = time.perf_counter_ns()
    count, total = algo.union_all(n, edges)
    t2 = time.perf_counter_ns()
    return f"{count}:{total}", (t1 - t0) / 1e6, (t2 - t1) / 1e6


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--size", type=int, required=True)
    ap.add_argument("--warm", type=int, default=1)
    args = ap.parse_args()
    if args.size <= 0 or args.warm < 0:
        print("--size must be positive and --warm not negative", file=sys.stderr)
        return 2
    start = time.perf_counter_ns()
    say(start, f"{platform.python_implementation()} {platform.python_version()}, kruskal, {args.size:,} nodes")
    say(start, f"building {4 * args.size:,} edges ...")
    t0 = time.perf_counter_ns()
    base = algo.edge_list(args.size)
    build_ms = (time.perf_counter_ns() - t0) / 1e6
    say(start, f"build done: {build_ms:,.1f} ms")
    for i in range(args.warm):
        say(start, f"warm-up run {i + 1}/{args.warm} (not timed) ...")
        timed_section(args.size, base.copy())
    edges = base.copy()
    say(start, "timed section: sort, then union-find ...")
    checksum, sort_ms, loop_ms = timed_section(args.size, edges)
    say(start, f"sort done: {sort_ms:,.1f} ms")
    say(start, f"union-find done: {loop_ms:,.1f} ms")
    say(start, f"timed section: {sort_ms + loop_ms:,.1f} ms (sort {sort_ms:,.1f} + loop {loop_ms:,.1f})")
    want = json.loads(EXPECTED.read_text()).get(f"kruskal@{args.size}")
    verdict = "ok" if checksum == want else f"NOT {want}" if want else "no expected value"
    say(start, f"checksum {checksum}: {verdict}")
    split = {"build_ms": round(build_ms, 3), "sort_ms": round(sort_ms, 3), "union_find_ms": round(loop_ms, 3)}
    print(
        json.dumps(
            {"kernel": "kruskal", "size": args.size, "checksum": checksum, "checksum_ok": checksum == want} | split
        ),
        flush=True,
    )
    return 0 if checksum == want else 1


if __name__ == "__main__":
    sys.exit(main())
