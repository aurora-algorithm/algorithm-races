"""meta.json: machine, versions of every runtime that ran, sizes and the measuring rules."""

import os
import platform
import subprocess
import sys
import time

from harness import session
from harness.config import ROOT, Runtime


def _cmd(argv: list[str], cwd=None) -> str:
    try:
        r = subprocess.run(argv, capture_output=True, text=True, timeout=120, cwd=cwd)
        return (r.stdout + r.stderr).strip()
    except (OSError, subprocess.TimeoutExpired) as e:
        return f"unavailable: {e}"


def cpu_name() -> str:
    if sys.platform == "darwin":
        return _cmd(["sysctl", "-n", "machdep.cpu.brand_string"])
    if sys.platform == "win32":
        return _cmd(["powershell", "-NoProfile", "-Command", "(Get-CimInstance Win32_Processor).Name"])
    return platform.processor()


def build_meta(runtimes: list[Runtime], kernels: dict, sizes: str, rounds: int, argv: list[str]) -> dict:
    meta = {
        "started": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "argv": argv,
        "sizes": sizes,
        "rounds": rounds,
        "machine": platform.node(),
        "os": platform.platform(),
        "cpu": cpu_name(),
        "logical_cpus": os.cpu_count(),
        "git_commit": _cmd(["git", "rev-parse", "HEAD"], cwd=ROOT),
        "git_dirty": bool(_cmd(["git", "status", "--porcelain", "--", "."], cwd=ROOT)),
        "rules": {
            "pinning": "1 thread: CPU 0; T threads: CPUs 0,2,..,2T-2 (one per P-core); none on macOS",
            "cold": "one process, --repeats 1: process wall/CPU/peak RSS + kernel time",
            "warm": f"one process, R = clamp(ceil({session.WARM_BUDGET_MS} ms / cold), {session.MIN_REPEATS}, "
            f"{session.MAX_REPEATS}), 2 if cold > {session.SLOW_COLD_MS} ms; median/min over repeats 2..R",
        },
        "kernels": {name: k[sizes] for name, k in kernels.items()},
        "runtimes": {},
    }
    if sys.platform == "win32":
        meta["power_plan"] = _cmd(["powercfg", "/getactivescheme"])
    for rt in runtimes:
        meta["runtimes"][rt.id] = {
            "label": rt.label,
            "argv": rt.argv,
            "env": rt.env,
            "version": _cmd(rt.version_argv, cwd=rt.cwd),
        }
    return meta
