"""kernels.json, runtimes.json and expected.json, with placeholders resolved for this machine.

Placeholders in runtimes.json: `{exe}` (".exe" on Windows), `{cmd}` (".cmd" on Windows), `{rid}` (the .NET runtime
identifier), `{env:NAME|fallback}` (an environment variable, else the fallback). `run_windows`, `version_windows`
override `run`, `version` there."""

import json
import os
import platform
import re
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IS_WIN = sys.platform == "win32"
_MSYS = re.compile(r"^/([a-zA-Z])/(.*)$")
_ENV = re.compile(r"\{env:([A-Z0-9_]+)\|([^}]*)\}")


def rid() -> str:
    arch = "arm64" if platform.machine().lower() in ("arm64", "aarch64") else "x64"
    os_part = "win" if IS_WIN else "osx" if sys.platform == "darwin" else "linux"
    return f"{os_part}-{arch}"


def native_path(value: str) -> str:
    """Git Bash exports paths like /c/Users/...; a native Windows process needs C:/Users/..."""
    m = _MSYS.match(value) if IS_WIN else None
    return f"{m.group(1).upper()}:/{m.group(2)}" if m else value


def resolve(text: str) -> str:
    text = _ENV.sub(lambda m: native_path(os.environ.get(m.group(1)) or m.group(2)), text)
    return (
        text.replace("{exe}", ".exe" if IS_WIN else "")
        .replace("{cmd}", ".cmd" if IS_WIN else "")
        .replace("{rid}", rid())
    )


@dataclass(frozen=True)
class Runtime:
    id: str
    lang: str
    label: str
    cwd: Path
    argv: list[str]
    version_argv: list[str]
    env: dict
    mt: bool
    kernels: list[str] | None
    build: str
    needs: list[str]
    groups: list[str] | None = None
    skip_groups: tuple[str, ...] = ()
    # A variant row (one setting or one swapped library): built and measured only when named with --runtime.
    opt_in: bool = False

    def runs(self, kernel: str, threads: int, spec: dict | None = None) -> bool:
        """`spec` is the kernel's kernels.json entry: its group, for library swaps the languages that have one, and
        `skip_runtimes` ({runtime id: why}) for a runtime of a listed language that cannot run it as described."""
        if self.kernels is not None and kernel not in self.kernels:
            return False
        if spec is not None:
            group = spec.get("group")
            if group in self.skip_groups or (self.groups is not None and group not in self.groups):
                return False
            if "langs" in spec and self.lang not in spec["langs"]:
                return False
            if self.id in spec.get("skip_runtimes", {}):
                return False
        return threads == 1 or self.mt

    def missing(self) -> str | None:
        """Why this runtime can't run here, or None."""
        for name in self.needs:
            if not os.environ.get(name):
                return f"${name} not set"
        exe = self.argv[0]
        path = Path(exe) if Path(exe).is_absolute() else self.cwd / exe
        if "/" in exe or "\\" in exe:
            return None if path.exists() else f"{path} not found (build it first)"
        return None if shutil.which(exe) else f"{exe} not on PATH"


def kernels() -> dict:
    return json.loads((ROOT / "kernels.json").read_text())


def expected() -> dict:
    return json.loads((ROOT / "expected.json").read_text())


def langs() -> dict:
    return json.loads((ROOT / "runtimes.json").read_text())["langs"]


def runtimes() -> list[Runtime]:
    cfg = json.loads((ROOT / "runtimes.json").read_text())
    out = []
    for r in cfg["runtimes"]:
        lang = cfg["langs"][r["lang"]]
        run = r.get("run_windows", r["run"]) if IS_WIN else r["run"]
        version = r.get("version_windows", r["version"]) if IS_WIN else r["version"]
        out.append(
            Runtime(
                id=r["id"],
                lang=r["lang"],
                label=r["label"],
                cwd=ROOT / lang["cwd"],
                argv=[resolve(a) for a in run],
                version_argv=[resolve(a) for a in version],
                env=r.get("env", {}),
                mt=r.get("mt", True),
                kernels=r.get("kernels"),
                build=resolve(r.get("build", "")),
                needs=r.get("needs", []),
                groups=r.get("groups"),
                skip_groups=tuple(r.get("skip_groups", ())),
                opt_in=r.get("opt_in", False),
            )
        )
    return out


def lang_build(lang: str) -> str:
    return resolve(langs()[lang]["build"])
