"""Starts one kernel process pinned to a set of CPUs and measures it: wall time from spawn to exit, CPU time (user +
system) and peak RSS (Windows: peak working set). Standard library only, so the i7's harness Python needs nothing."""

import os
import subprocess
import sys
import time
from contextlib import contextmanager
from dataclasses import dataclass

IS_WIN = sys.platform == "win32"
TIMEOUT_S = 3600


@dataclass(frozen=True)
class ProcResult:
    returncode: int
    stdout: str
    stderr: str
    wall_ms: float
    cpu_ms: float | None
    peak_rss_mb: float | None


if IS_WIN:
    import ctypes
    from ctypes import wintypes

    class _MemCounters(ctypes.Structure):
        _fields_ = [
            ("cb", wintypes.DWORD),
            ("PageFaultCount", wintypes.DWORD),
            ("PeakWorkingSetSize", ctypes.c_size_t),
            ("WorkingSetSize", ctypes.c_size_t),
            ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
            ("QuotaPagedPoolUsage", ctypes.c_size_t),
            ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
            ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
            ("PagefileUsage", ctypes.c_size_t),
            ("PeakPagefileUsage", ctypes.c_size_t),
        ]

    _k32 = ctypes.WinDLL("kernel32", use_last_error=True)
    _k32.GetCurrentProcess.restype = wintypes.HANDLE
    _k32.SetProcessAffinityMask.argtypes = [wintypes.HANDLE, ctypes.c_size_t]
    _k32.GetProcessAffinityMask.argtypes = [
        wintypes.HANDLE,
        ctypes.POINTER(ctypes.c_size_t),
        ctypes.POINTER(ctypes.c_size_t),
    ]
    _k32.GetProcessTimes.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
    _k32.K32GetProcessMemoryInfo.argtypes = [wintypes.HANDLE, ctypes.POINTER(_MemCounters), wintypes.DWORD]

    def _ft(ft) -> int:
        return (ft.dwHighDateTime << 32) | ft.dwLowDateTime

    def _win_usage(handle) -> tuple[float, float]:
        c, e, k, u = (wintypes.FILETIME() for _ in range(4))
        if not _k32.GetProcessTimes(handle, *(ctypes.byref(x) for x in (c, e, k, u))):
            raise OSError(ctypes.get_last_error(), "GetProcessTimes")
        mem = _MemCounters()
        mem.cb = ctypes.sizeof(mem)
        if not _k32.K32GetProcessMemoryInfo(handle, ctypes.byref(mem), mem.cb):
            raise OSError(ctypes.get_last_error(), "GetProcessMemoryInfo")
        return (_ft(k) + _ft(u)) / 1e4, mem.PeakWorkingSetSize / 2**20


@contextmanager
def pinned(cpus: list[int] | None):
    """The child inherits this process's affinity on Windows and Linux, from its first instruction, so runtimes that
    size themselves at start (.NET, the JVM, Go) see only these CPUs. macOS has no affinity API: no pinning there."""
    if not cpus:
        yield "none"
        return
    if IS_WIN:
        me = _k32.GetCurrentProcess()
        old, system = ctypes.c_size_t(), ctypes.c_size_t()
        _k32.GetProcessAffinityMask(me, ctypes.byref(old), ctypes.byref(system))
        if not _k32.SetProcessAffinityMask(me, sum(1 << c for c in cpus)):
            raise OSError(ctypes.get_last_error(), "SetProcessAffinityMask")
        try:
            yield ",".join(map(str, cpus))
        finally:
            _k32.SetProcessAffinityMask(me, old.value)
    elif hasattr(os, "sched_setaffinity"):
        old = os.sched_getaffinity(0)
        os.sched_setaffinity(0, cpus)
        try:
            yield ",".join(map(str, cpus))
        finally:
            os.sched_setaffinity(0, old)
    else:
        yield "none (no affinity API on this OS)"


def run(argv: list[str], cwd: str, env: dict, cpus: list[int] | None) -> tuple[ProcResult, str]:
    with pinned(cpus) as pin:
        t0 = time.perf_counter()
        p = subprocess.Popen(argv, cwd=cwd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if IS_WIN:
        try:
            out, err = p.communicate(timeout=TIMEOUT_S)
        except subprocess.TimeoutExpired:
            p.kill()
            out, err = p.communicate()
        wall = (time.perf_counter() - t0) * 1000
        cpu, rss = _win_usage(int(p._handle))
        return ProcResult(p.returncode, out, err, wall, cpu, rss), pin
    return _posix_wait(p, t0), pin


def _posix_wait(p: subprocess.Popen, t0: float) -> ProcResult:
    """Reads the pipes on threads, reaps the child with wait4 for its rusage (Popen.wait would discard it)."""
    import threading

    chunks: dict[str, str] = {}

    def drain(name, stream):
        chunks[name] = stream.read()

    readers = [threading.Thread(target=drain, args=(n, s)) for n, s in (("out", p.stdout), ("err", p.stderr))]
    for r in readers:
        r.start()
    killer = threading.Timer(TIMEOUT_S, p.kill)
    killer.start()
    _, status, usage = os.wait4(p.pid, 0)
    killer.cancel()
    wall = (time.perf_counter() - t0) * 1000
    for r in readers:
        r.join()
    p.returncode = os.waitstatus_to_exitcode(status)
    rss_unit = 1 if sys.platform == "darwin" else 1024
    return ProcResult(
        p.returncode,
        chunks.get("out", ""),
        chunks.get("err", ""),
        wall,
        (usage.ru_utime + usage.ru_stime) * 1000,
        usage.ru_maxrss * rss_unit / 2**20,
    )
