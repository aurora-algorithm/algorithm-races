"""Full-session time from a probe session (sizes "probe", about a tenth of the full work): each cell's kernel time
is scaled by its cost model, its setup (startup + input + checksum = process time - kernel time) by its input size,
then the warm-repeat rule of session.py is applied. Cache effects at the full sizes (matmul, hashmap) make this an
underestimate for the compiled languages, so a margin is added on top."""

import math

from harness.session import warm_repeats

FACT = math.factorial


def work(model: str, n: int) -> float:
    if model == "nlogn":
        return n * math.log2(n)
    if model == "n":
        return float(n)
    if model == "n2":
        return float(n) * n
    if model == "n3":
        return float(n) ** 3
    if model == "bt":
        return 2.0**n * ((n - 4) // 2 + 1 + 4)
    if model == "fact":
        return float(FACT(n) * n)
    if model == "const":
        return 1.0
    raise ValueError(model)


def cell_seconds(row: dict, k: dict) -> float:
    kernel_ms = row["cold"]["kernel_ms"]
    setup_ms = max(row["cold"]["wall_ms"] - kernel_ms, 0.0)
    full_kernel = kernel_ms * work(k["cost"], k["full"]) / work(k["cost"], k["probe"])
    full_setup = setup_ms * work(k["input"], k["full"]) / work(k["input"], k["probe"])
    repeats = warm_repeats(full_kernel, quick=False)
    return ((full_setup + full_kernel) + (full_setup + repeats * full_kernel)) / 1000


def estimate(rows: list[dict], kernels: dict, speed: float, rounds: int, margin: float) -> dict:
    per_rt: dict[str, float] = {}
    per_kernel: dict[str, float] = {}
    for r in rows:
        if r.get("round") != 1 or "kernel_ms" not in r["cold"]:
            continue
        s = cell_seconds(r, kernels[r["kernel"]]) * speed
        per_rt[r["runtime"]] = per_rt.get(r["runtime"], 0.0) + s
        per_kernel[r["kernel"]] = per_kernel.get(r["kernel"], 0.0) + s
    measured = sum(per_rt.values()) * rounds
    return {
        "measure_s": measured,
        "with_margin_s": measured * margin,
        "per_runtime_s": dict(sorted(per_rt.items(), key=lambda x: -x[1])),
        "per_kernel_s": dict(sorted(per_kernel.items(), key=lambda x: -x[1])),
    }
