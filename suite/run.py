#!/usr/bin/env python3
"""The kernels track (kernels/SPEC.md). Standard library only (Python 3.11+).

run.py build    [--runtime ID ...]           build every language (and AOT/native extras)
run.py lint     [--lang L ...]               formatters + linters, zero warnings
run.py bench    [--quick | --probe] [--session NAME] [--rounds N] [--kernel K ...] [--group G ...]
                [--runtime ID ...] [--threads T ...] [--warm-repeats R]
                                             kernels and runtimes marked "opt_in" run only when named
                                             measure; results/<session>/kernels.jsonl, summary.csv, meta.json;
run.py summary  SESSION_DIR                  rewrite summary.csv and print the table
run.py estimate SESSION_DIR [--speed X]      full-session time from a --probe session
run.py expect   SESSION_DIR                  add the session's agreed checksums to expected.json
"""

import argparse
import json
import os
import platform
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from harness import config, estimate, meta, session, summary

RESULTS = config.ROOT / "results"


def bash_exe() -> str:
    """Git Bash on Windows, never C:/Windows/System32/bash.exe (that one is WSL)."""
    if os.environ.get("BASH_EXE"):
        return os.environ["BASH_EXE"]
    if config.IS_WIN:
        for p in ("C:/Program Files/Git/bin/bash.exe", "C:/Program Files/Git/usr/bin/bash.exe"):
            if Path(p).exists():
                return p
        raise RuntimeError("Git Bash not found; set BASH_EXE")
    return "bash"


def bash(cmd: str, cwd: Path) -> int:
    print(f"$ (cd {cwd.relative_to(config.ROOT)} && {cmd})", flush=True)
    return subprocess.call([bash_exe(), "-c", cmd], cwd=cwd)


def cmd_build(args) -> int:
    rts = [r for r in config.runtimes() if (r.id in args.runtime if args.runtime else not r.opt_in)]
    failed = []
    for lang in dict.fromkeys(r.lang for r in rts):
        cmd = config.lang_build(lang)
        if cmd and bash(cmd, config.ROOT / config.langs()[lang]["cwd"]):
            failed.append(lang)
    for rt in rts:
        if not rt.build:
            continue
        if any(not os.environ.get(n) for n in rt.needs):
            print(f"skip {rt.id}: needs {', '.join(rt.needs)}")
            continue
        if bash(rt.build, rt.cwd):
            failed.append(rt.id)
    print("build failed: " + ", ".join(failed) if failed else "build ok")
    return 1 if failed else 0


def cmd_lint(args) -> int:
    failed = []
    for lang, cfg in config.langs().items():
        if args.lang and lang not in args.lang:
            continue
        for cmd in cfg.get("lint", []):
            if bash(config.resolve(cmd), config.ROOT / cfg["cwd"]):
                failed.append(f"{lang}: {cmd}")
    print("lint failed:\n  " + "\n  ".join(failed) if failed else "lint clean")
    return 1 if failed else 0


def cmd_bench(args) -> int:
    sizes = "quick" if args.quick else "probe" if args.probe else "full"
    kernels = config.kernels()
    rts = config.runtimes()
    cells, skipped = session.plan(kernels, rts, sizes, args.kernel, args.runtime, args.threads, args.group)
    for rt, why in skipped:
        print(f"skip {rt}: {why}")
    if not cells:
        print("nothing to run")
        return 1
    name = args.session or f"{time.strftime('%Y-%m-%d-%H%M')}-{sizes}-{platform.node().split('.')[0]}"
    out = RESULTS / name
    out.mkdir(parents=True, exist_ok=True)
    used = [r for r in rts if r.id in {c.runtime.id for c in cells}]
    meta_path = out / "meta.json"
    m = (
        json.loads(meta_path.read_text())
        if meta_path.exists()
        else meta.build_meta(used, kernels, sizes, args.rounds, sys.argv)
    )
    meta_path.write_text(json.dumps(m, indent=1))
    print(f"{len(cells)} cells x {args.rounds} round(s) -> {out}")
    jsonl = session.run_session(out, cells, config.expected(), args.quick, args.rounds, fixed_repeats=args.warm_repeats)
    m["finished"] = time.strftime("%Y-%m-%dT%H:%M:%S%z")
    meta_path.write_text(json.dumps(m, indent=1))
    return print_summary(jsonl)


def print_summary(jsonl: Path) -> int:
    rows = summary.summarize(summary.load(jsonl))
    summary.write(rows, jsonl.parent / "summary.csv")
    print(summary.table(rows))
    bad = [r for r in rows if not r["checksum_ok"]]
    print(
        f"\n{len(rows)} cells, {len(bad)} not ok"
        + (": " + ", ".join(f"{r['kernel']}/T{r['threads']}/{r['runtime']}" for r in bad) if bad else "")
    )
    return 1 if bad else 0


def cmd_summary(args) -> int:
    return print_summary(Path(args.session_dir) / "kernels.jsonl")


def cmd_estimate(args) -> int:
    rows = summary.load(Path(args.session_dir) / "kernels.jsonl")
    est = estimate.estimate(rows, config.kernels(), args.speed, args.rounds, args.margin)
    total = est["with_margin_s"] + args.build_min * 60
    print(
        f"measure: {est['measure_s'] / 3600:.2f} h at speed x{args.speed}, {args.rounds} round(s); "
        f"with x{args.margin} margin + {args.build_min} min build/verify: {total / 3600:.2f} h"
    )
    print("per runtime (min): " + ", ".join(f"{k} {v / 60:.0f}" for k, v in est["per_runtime_s"].items()))
    print("per kernel (min):  " + ", ".join(f"{k} {v / 60:.0f}" for k, v in est["per_kernel_s"].items()))
    return 0


def cmd_expect(args) -> int:
    rows = summary.load(Path(args.session_dir) / "kernels.jsonl")
    agreed: dict[str, set] = {}
    for r in rows:
        if r.get("checksum"):
            agreed.setdefault(f"{r['kernel']}@{r['size']}", set()).add(r["checksum"])
    exp = config.expected()
    for key, sums in sorted(agreed.items()):
        if len(sums) != 1:
            print(f"{key}: runtimes disagree {sorted(sums)}; not added")
        elif key in exp and exp[key] != next(iter(sums)):
            print(f"{key}: {next(iter(sums))} differs from expected {exp[key]}; not changed")
        else:
            exp[key] = next(iter(sums))
    (config.ROOT / "expected.json").write_text(json.dumps(exp, indent=1) + "\n")
    print(f"expected.json: {len(exp)} entries")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    b = sub.add_parser("build")
    b.add_argument("--runtime", action="append")
    li = sub.add_parser("lint")
    li.add_argument("--lang", action="append")
    be = sub.add_parser("bench")
    size = be.add_mutually_exclusive_group()
    size.add_argument("--quick", action="store_true")
    size.add_argument("--probe", action="store_true")
    be.add_argument("--session")
    be.add_argument("--rounds", type=int, default=1)
    be.add_argument("--kernel", action="append")
    be.add_argument("--group", action="append", help="sort, bg, lib, loop, mt, work")
    be.add_argument("--runtime", action="append")
    be.add_argument("--threads", action="append", type=int)
    be.add_argument("--warm-repeats", type=int, help="repeats in every warm process (default: a 10 s budget, 3-10)")
    be.add_argument("--record-rounds", type=int, default=0, help="1: record a hyperfine race per kernel afterwards")
    be.add_argument("--record-runs", type=int, default=2, help="hyperfine runs per runtime in a recorded race")
    s = sub.add_parser("summary")
    s.add_argument("session_dir")
    e = sub.add_parser("estimate")
    e.add_argument("session_dir")
    e.add_argument("--speed", type=float, default=1.2, help="i7 time / this machine's time (default 1.2)")
    e.add_argument("--rounds", type=int, default=1)
    e.add_argument("--margin", type=float, default=1.25)
    e.add_argument("--build-min", type=float, default=25)
    x = sub.add_parser("expect")
    x.add_argument("session_dir")
    args = ap.parse_args()
    return {
        "build": cmd_build,
        "lint": cmd_lint,
        "bench": cmd_bench,
        "summary": cmd_summary,
        "estimate": cmd_estimate,
        "expect": cmd_expect,
    }[args.cmd](args)


if __name__ == "__main__":
    sys.exit(main())
