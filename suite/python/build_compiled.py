"""Builds the compiled Python runtimes (kernels/SPEC.md section 14) from the unchanged kernel sources.

usage: python build_compiled.py mypyc|cython|nuitka [--force]
Run it with the interpreter that will run the result (../.venv-pyc).
mypyc and cython: copies the kernel modules to bin/<tool>/, compiles each to an extension module there, removes the
copied .py files (so only the extensions can be imported) and lists the compiled modules in bin/<tool>/modules.txt.
mypyc only compiles a module that type-checks (mypy, stock settings); the others are left out and named.
nuitka: a standalone build of compiled.py and every module it imports, bin/nuitka/compiled.dist/kernels[.exe].
Windows needs MSVC (Visual Studio Build Tools) for all three.
"""

import re
import shutil
import subprocess
import sys
from pathlib import Path

from compiled import HERE, MODULES

BIN = HERE / "bin"


def copy_sources(tool: str) -> Path:
    out = BIN / tool
    shutil.rmtree(out, ignore_errors=True)
    out.mkdir(parents=True)
    for name in MODULES:
        shutil.copy2(HERE / f"{name}.py", out / f"{name}.py")
    return out


def finish(out: Path, built: list[str]) -> None:
    for name in MODULES:
        (out / f"{name}.py").unlink()
    shutil.rmtree(out / "build", ignore_errors=True)
    (out / "modules.txt").write_text("\n".join(built) + "\n")


def type_checks(out: Path, name: str) -> bool:
    r = subprocess.run([sys.executable, "-m", "mypy", f"{name}.py"], cwd=out, capture_output=True, text=True)
    if r.returncode:
        print(f"mypyc: leaving {name} out, it does not type-check:\n{r.stdout}", flush=True)
    return r.returncode == 0


def build_mypyc(out: Path) -> list[str]:
    return [name for name in MODULES if type_checks(out, name)]


def crashed_modules(output: str, built: list[str]) -> list[str]:
    """Modules a compiler reported an internal error in (mypyc: "puzzle.py:54: ... internal mypyc error")."""
    hits = re.findall(r"^(\w+)\.py:\d+: .*$\n^\1\.py:\d+: note: this is an internal mypyc error", output, re.M)
    return [name for name in dict.fromkeys(hits) if name in built]


def build_cython(_out: Path) -> list[str]:
    return list(MODULES)


COMPILERS = {
    "mypyc": [sys.executable, "-m", "mypyc"],
    "cython": [sys.executable, "-m", "Cython.Build.Cythonize", "-i", "-3"],
}


def compile_modules(tool: str, out: Path, built: list[str]) -> tuple[int, list[str]]:
    """Compiles `built`; a module the compiler crashes on is left out and the rest compiled again."""
    while True:
        argv = [*COMPILERS[tool], *(f"{name}.py" for name in built)]
        r = subprocess.run(argv, cwd=out, capture_output=True, text=True)
        print(r.stdout + r.stderr, end="", flush=True)
        crashed = crashed_modules(r.stdout + r.stderr, built) if r.returncode else []
        if not crashed:
            return r.returncode, built
        print(f"{tool}: leaving {' '.join(crashed)} out, the compiler crashed on it", flush=True)
        built = [name for name in built if name not in crashed]


def build_nuitka() -> list[str]:
    argv = [sys.executable, "-m", "nuitka", "--mode=standalone", "--assume-yes-for-downloads"]
    exe = "kernels.exe" if sys.platform == "win32" else "kernels"
    argv += ["--output-dir=bin/nuitka", f"--output-filename={exe}", "--remove-output"]
    if sys.platform == "win32":
        argv.append("--msvc=latest")
    return [*argv, "compiled.py"]


def up_to_date(tool: str) -> bool:
    """The build is newer than every source it came from (run.py build runs before every session; Nuitka takes
    minutes). --force rebuilds anyway."""
    stamp = BIN / tool / ("compiled.dist" if tool == "nuitka" else "modules.txt")
    names = [*MODULES, "main", "work", "compiled"]
    sources = [*(HERE / f"{name}.py" for name in names), HERE / "requirements-pyc.txt"]
    return stamp.exists() and stamp.stat().st_mtime > max(f.stat().st_mtime for f in sources)


def main() -> int:
    force = "--force" in sys.argv
    args = [a for a in sys.argv[1:] if a != "--force"]
    tool = args[0] if len(args) == 1 else ""
    if tool in ("mypyc", "cython", "nuitka") and not force and up_to_date(tool):
        print(f"{tool}: bin/{tool} is up to date")
        return 0
    if tool == "nuitka":
        shutil.rmtree(BIN / "nuitka", ignore_errors=True)
        return subprocess.call(build_nuitka(), cwd=HERE)
    pick = {"mypyc": build_mypyc, "cython": build_cython}
    if tool not in pick:
        print("usage: build_compiled.py mypyc|cython|nuitka [--force]", file=sys.stderr)
        return 2
    out = copy_sources(tool)
    code, built = compile_modules(tool, out, pick[tool](out))
    if code == 0:
        finish(out, built)
    return code


if __name__ == "__main__":
    sys.exit(main())
