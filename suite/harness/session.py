"""One session: every (kernel, threads, runtime) cell, a cold process then a warm one (kernels/SPEC.md section 9)."""

import json
import math
import os
import shutil
import statistics
import sys
import time
from dataclasses import dataclass
from pathlib import Path

from harness import data, procs
from harness.config import ROOT, Runtime, kernels

WARM_BUDGET_MS = 10_000
SLOW_COLD_MS = 30_000
MIN_REPEATS, MAX_REPEATS = 3, 10
QUICK_REPEATS = 3
# Groups whose every cell must have an expected.json checksum at its size.
EXPECTED_REQUIRED = frozenset({"builtins"})


@dataclass(frozen=True)
class Cell:
    kernel: str
    group: str
    size: int
    threads: int
    runtime: Runtime


def cpus_for(threads: int) -> list[int] | None:
    """CPU 0, 2, 4, ...: one logical CPU per physical P-core of the i7 (SMT siblings stay idle)."""
    if sys.platform == "darwin":
        return None
    return list(range(0, 2 * threads, 2))


def warm_repeats(cold_kernel_ms: float, quick: bool, fixed: int | None = None) -> int:
    if fixed:
        return fixed
    if quick:
        return QUICK_REPEATS
    if cold_kernel_ms > SLOW_COLD_MS:
        return 2
    return max(MIN_REPEATS, min(MAX_REPEATS, math.ceil(WARM_BUDGET_MS / max(cold_kernel_ms, 1.0))))


def argv_for(cell: Cell, repeats: int) -> list[str]:
    argv = list(cell.runtime.argv)
    exe = argv[0]
    if "/" not in exe and "\\" not in exe:
        argv[0] = shutil.which(exe) or exe
    elif not Path(exe).is_absolute():
        argv[0] = os.path.normpath(cell.runtime.cwd / exe)
    return [
        *argv,
        cell.kernel,
        "--size",
        str(cell.size),
        "--repeats",
        str(repeats),
        "--threads",
        str(cell.threads),
        "--data",
        str(ROOT / "data"),
    ]


def parse(res: procs.ProcResult) -> dict | None:
    for line in reversed(res.stdout.strip().splitlines()):
        if line.startswith("{"):
            try:
                return json.loads(line)
            except ValueError:
                return None
    return None


def one_process(cell: Cell, repeats: int, raw_dir: Path, tag: str) -> dict:
    env = dict(os.environ, **cell.runtime.env)
    res, pin = procs.run(argv_for(cell, repeats), str(cell.runtime.cwd), env, cpus_for(cell.threads))
    out = parse(res)
    row = {
        "exit": res.returncode,
        "wall_ms": round(res.wall_ms, 2),
        "cpu_ms": _r(res.cpu_ms),
        "peak_rss_mb": _r(res.peak_rss_mb),
        "pinned": pin,
    }
    if out is None or res.returncode != 0:
        raw_dir.mkdir(parents=True, exist_ok=True)
        name = f"{cell.kernel}__{cell.runtime.id}__t{cell.threads}__{tag}.txt"
        (raw_dir / name).write_text(
            f"argv: {argv_for(cell, repeats)}\n--- stdout\n{res.stdout}\n--- stderr\n{res.stderr}"
        )
        row["error"] = (res.stderr.strip().splitlines() or ["no output"])[-1][:300]
        return row
    row.update(checksum=out.get("checksum"), times_ms=out.get("times_ms", []), runtime_reported=out.get("runtime"))
    if out.get("extra"):
        row["extra"] = out["extra"]
    return row


def _r(x: float | None) -> float | None:
    return None if x is None else round(x, 2)


def run_cell(
    cell: Cell, expected: dict, quick: bool, raw_dir: Path, round_no: int, fixed_repeats: int | None = None
) -> dict:
    cold = one_process(cell, 1, raw_dir, f"r{round_no}-cold")
    row = {
        "kernel": cell.kernel,
        "group": cell.group,
        "size": cell.size,
        "threads": cell.threads,
        "runtime": cell.runtime.id,
        "lang": cell.runtime.lang,
        "label": cell.runtime.label,
        "round": round_no,
        "ts": time.strftime("%Y-%m-%dT%H:%M:%S"),
        "cold": cold,
    }
    if "error" in cold:
        row["ok"] = False
        return row
    cold["kernel_ms"] = cold["times_ms"][0]
    repeats = warm_repeats(cold["kernel_ms"], quick, fixed_repeats)
    warm = one_process(cell, repeats, raw_dir, f"r{round_no}-warm")
    row["warm"] = warm
    if "error" not in warm:
        t = warm["times_ms"]
        rest = t[1:] or t
        warm.update(repeats=len(t), first_ms=t[0], median_ms=round(statistics.median(rest), 3), min_ms=min(rest))
    want = expected.get(f"{cell.kernel}@{cell.size}")
    sums = {cold.get("checksum"), warm.get("checksum")}
    row["checksum"] = cold.get("checksum")
    row["expected"] = want
    # Every builtins way has an expected.json entry at every size (sync.py --expect); a missing one means the
    # entry was never generated, so the cell cannot prove it agrees with the other ways.
    unchecked = want is None and cell.group in EXPECTED_REQUIRED
    if unchecked:
        row["missing_expected"] = True
    agrees = want is None or row["checksum"] == want
    row["ok"] = "error" not in warm and len(sums) == 1 and agrees and not unchecked
    return row


def plan(kernels: dict, runtimes: list[Runtime], sizes: str, only_kernels, only_runtimes, threads_filter, groups=None):
    cells, skipped = [], []
    for name, k in kernels.items():
        if only_kernels and name not in only_kernels:
            continue
        if k.get("opt_in") and not only_kernels:
            continue
        if groups and k["group"] not in groups:
            continue
        for threads in k.get("threads", [1]):
            if threads_filter and threads not in threads_filter:
                continue
            for rt in runtimes:
                if only_runtimes and rt.id not in only_runtimes:
                    continue
                if rt.opt_in and not only_runtimes:
                    continue
                if rt.id in k.get("skip_runtimes", {}):
                    skipped.append((f"{rt.id} {name}", k["skip_runtimes"][rt.id]))
                    continue
                if not rt.runs(name, threads, k):
                    continue
                why = rt.missing()
                if why:
                    skipped.append((rt.id, why))
                    continue
                cells.append(Cell(name, k["group"], k[sizes], threads, rt))
    return cells, sorted(set(skipped))


def rotate(cells: list[Cell], round_no: int) -> list[Cell]:
    """Same kernel order every round; the runtime order within a kernel shifts by one per round."""
    out, i = [], 0
    while i < len(cells):
        j = i
        while j < len(cells) and (cells[j].kernel, cells[j].threads) == (cells[i].kernel, cells[i].threads):
            j += 1
        block = cells[i:j]
        shift = round_no % len(block)
        out += block[shift:] + block[:shift]
        i = j
    return out


def run_session(
    out_dir: Path,
    cells: list[Cell],
    expected: dict,
    quick: bool,
    rounds: int,
    log=print,
    fixed_repeats: int | None = None,
) -> Path:
    """fixed_repeats: the warm process runs exactly that many repeats (default: WARM_BUDGET_MS decides)."""
    out_dir.mkdir(parents=True, exist_ok=True)
    jsonl = out_dir / "kernels.jsonl"
    done = _done(jsonl)
    specs = kernels()
    for name, size in sorted({(c.kernel, c.size) for c in cells}):
        data.ensure(name, specs[name], size)
    total = len(cells) * rounds
    n = 0
    for round_no in range(1, rounds + 1):
        for cell in rotate(cells, round_no - 1):
            n += 1
            key = (cell.kernel, cell.threads, cell.runtime.id, round_no)
            if key in done:
                continue
            row = run_cell(cell, expected, quick, out_dir / "raw", round_no, fixed_repeats)
            with jsonl.open("a", encoding="utf-8") as f:
                f.write(json.dumps(row) + "\n")
            w = row.get("warm", {})
            status = "ok " if row["ok"] else "BAD"
            detail = w.get("error") or row["cold"].get("error") or f"median {w.get('median_ms')} ms"
            if row.get("missing_expected"):
                detail += f", no expected.json entry for {cell.kernel}@{cell.size}"
            log(
                f"[{n}/{total}] {status} {cell.kernel:16} T={cell.threads} {cell.runtime.id:12} {detail} "
                f"(process {row['cold']['wall_ms']:.0f} ms, {row['cold'].get('peak_rss_mb')} MB)"
            )
    return jsonl


def _done(jsonl: Path) -> set:
    if not jsonl.exists():
        return set()
    keys = set()
    for line in jsonl.read_text(encoding="utf-8").splitlines():
        r = json.loads(line)
        keys.add((r["kernel"], r["threads"], r["runtime"], r["round"]))
    return keys
