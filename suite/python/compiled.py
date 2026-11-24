"""main.py with the kernel modules compiled ahead of time (kernels/SPEC.md section 14).

usage: python compiled.py mypyc|cython KERNEL --size N ...   kernel modules from bin/<tool>/ (build_compiled.py)
       bin/nuitka/compiled.dist/kernels KERNEL --size N ...  this file and every module compiled by Nuitka
main.py itself stays the same driver; the runtime field names the compiler, and the run fails if a kernel module
was not loaded from a compiled extension, so a missing build never passes as a compiled result.
"""

import importlib
import importlib.machinery
import sys
from importlib.metadata import version
from pathlib import Path

HERE = Path(__file__).resolve().parent
MODULES = ("algo", "bg", "gen", "misc", "mt", "puzzle", "sorts", "sudoku")
TOOLS = {"mypyc": "mypy", "cython": "cython"}


def nuitka_info() -> str:
    c = __compiled__  # type: ignore[name-defined]  # noqa: F821 (set by Nuitka in every compiled module)
    mode = "standalone" if c.standalone else "accelerated"
    return f"nuitka {c.major}.{c.minor}.{c.micro} {mode}"


def load_compiled(tool: str) -> str:
    """Puts bin/<tool>/ first on the path; a module the build left out (modules.txt) runs as plain Python."""
    out = HERE / "bin" / tool
    if not (out / "modules.txt").exists():
        raise SystemExit(f"{out} has no build: run build_compiled.py {tool}")
    built = (out / "modules.txt").read_text().split()
    sys.path.insert(0, str(out))
    for name in built:
        path = importlib.import_module(name).__file__ or ""
        if not path.endswith(tuple(importlib.machinery.EXTENSION_SUFFIXES)):
            raise SystemExit(f"{name} loaded from {path}, not a {tool} extension: run build_compiled.py {tool}")
    plain = [name for name in MODULES if name not in built]
    return f"{tool} {version(TOOLS[tool])}" + (f" (not compiled: {' '.join(plain)})" if plain else "")


def main() -> int:
    if "__compiled__" in globals():
        info = nuitka_info()
    else:
        if len(sys.argv) < 2 or sys.argv[1] not in TOOLS:
            print(f"usage: compiled.py {'|'.join(TOOLS)} KERNEL ...", file=sys.stderr)
            return 2
        info = load_compiled(sys.argv.pop(1))
    import main as driver

    plain = driver.runtime_info
    driver.runtime_info = lambda use_numpy, kernel: f"{plain(use_numpy, kernel)} {info}"
    return driver.main()


if __name__ == "__main__":
    sys.exit(main())
