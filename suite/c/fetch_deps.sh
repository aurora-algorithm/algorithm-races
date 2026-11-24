#!/bin/bash
# Downloads the C libraries pinned in deps.lock into deps/ and checks each download's SHA-256.
# A folder that is already there is kept. Needs curl and tar (Git Bash on Windows: curl, and Windows' own
# C:/Windows/System32/tar.exe, which also reads the .tar.bz2 of Mbed TLS).
#
# usage (from kernels/c):  ./fetch_deps.sh
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p deps/_downloads

TAR=tar
if [ "${OS:-}" = "Windows_NT" ] && [ -x /c/Windows/System32/tar.exe ]; then TAR=/c/Windows/System32/tar.exe; fi

sha256_of() {
  if command -v sha256sum >/dev/null; then sha256sum "$1" | cut -d' ' -f1; else shasum -a 256 "$1" | cut -d' ' -f1; fi
}

while read -r name version folder sha url; do
  case "$name" in '' | '#'*) continue ;; esac
  if [ -d "deps/$folder" ]; then
    continue
  fi
  file="deps/_downloads/${url##*/}"
  echo "fetch $name $version"
  curl -fsSL --retry 3 -o "$file" "$url"
  got="$(sha256_of "$file")"
  if [ "$got" != "$sha" ]; then
    echo "$name: sha256 $got, deps.lock says $sha" >&2
    rm -f "$file"
    exit 1
  fi
  (cd deps && "$TAR" -xf "_downloads/${url##*/}")
  [ -d "deps/$folder" ] || { echo "$name: $folder missing after unpacking" >&2; exit 1; }
done < deps.lock
echo "deps ok"
