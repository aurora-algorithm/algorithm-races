#!/bin/bash
# Builds the Goose kernels with one C backend: `goose build` with its default settings (-O2, the C compiler's own
# /O2 or -O2, the runtime linked as an object), only the compiler named. Without --cc, goose on Windows takes the
# first of cl on PATH, Visual Studio's clang-cl, then MSVC via vcvars64, so a row always names its compiler.
#
# usage (from kernels/goose):  ./build.sh msvc|clang|tcc
#   msvc   MSVC through Visual Studio's vcvars64 (Windows only)  -> bin/kernels-msvc.exe
#   clang  Visual Studio's bundled clang-cl on Windows, clang elsewhere  -> bin/kernels-clang[.exe]
#   tcc    nothing to build (goose run compiles in-process with TinyCC); type-checks the sources
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p bin
G="${GOOSE:-goose}"
EXE=""
WIN=0
if [ "${OS:-}" = "Windows_NT" ]; then EXE=".exe"; WIN=1; fi
case "${1:-}" in
  msvc)
    [ "$WIN" = 1 ] || { echo "goose-msvc builds on Windows only" >&2; exit 1; }
    "$G" build --cc msvc -o "bin/kernels-msvc$EXE" main.goose ;;
  clang)
    CC=clang
    if [ "$WIN" = 1 ]; then CC=clang-cl; fi
    "$G" build --cc "$CC" -o "bin/kernels-clang$EXE" main.goose ;;
  tcc)
    "$G" check main.goose ;;
  *)
    echo "usage: $0 msvc|clang|tcc" >&2; exit 2 ;;
esac
