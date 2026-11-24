#!/bin/bash
# Builds the C kernels with one compiler, stock release flags, libraries from deps/ (./fetch_deps.sh) compiled
# by the same compiler with the same optimisation flag. Output: bin/<cc>/kernels[.exe].
#
# usage (from kernels/c):
#   ./build.sh msvc    cl -O2 -DNDEBUG (Windows only; vcvars64.bat from $VCVARS64, else $CPP_VCVARS, else VS 2022 Community)
#   ./build.sh clang   clang -O3 -DNDEBUG ($CC_CLANG_HOME/bin/clang, else clang beside $CPP_CLANG, else clang on PATH; on Windows it
#                      targets x86_64-pc-windows-msvc and needs the vcvars64 environment as well)
#   ./build.sh clang-o2  the clang build at -O2, the one flag `goose build` passes to clang, for Goose-vs-C rows
#                      -> bin/clang-o2/kernels
#   ./build.sh gcc     gcc -O3 -DNDEBUG ($WINLIBS_HOME/bin/gcc, else gcc beside $CPP_GCC, else $GCC, gcc-16 or gcc)
#   ./build.sh lint    clang-format check, then our sources with every compiler found here at
#                      -Wall -Wextra -Werror (gcc, clang) and -W4 -WX (MSVC)
set -euo pipefail
cd "$(dirname "$0")"

WIN=0
EXE=""
if [ "${OS:-}" = "Windows_NT" ]; then WIN=1; EXE=".exe"; fi
CLANG_FORMAT_VERSION=21.1.8
VCVARS64_DEFAULT="C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat"

lock_folder() { awk -v n="$1" '$1 == n { print "deps/" $3 }' deps.lock; }
lock_version() { awk -v n="$1" '$1 == n { print $2 }' deps.lock; }

ZLIB=$(lock_folder zlib)
ZSTD=$(lock_folder zstd)
PCRE2=$(lock_folder pcre2)
MBEDTLS=$(lock_folder mbedtls)
CJSON=$(lock_folder cjson)
ARGON2=$(lock_folder argon2)
BLOWFISH=$(lock_folder crypt_blowfish)
LIBCSV=$(lock_folder libcsv)
STB=$(lock_folder stb)
STB_RESIZE=$(lock_folder stb_resize)

# Imports INCLUDE, LIB, LIBPATH and PATH from vcvars64.bat into this shell (MSVC, and clang's MSVC target).
msvc_env() {
  [ "$WIN" = 1 ] || return 0
  command -v cl.exe >/dev/null 2>&1 && return 0
  local vc="${VCVARS64:-${CPP_VCVARS:-$VCVARS64_DEFAULT}}"
  [ -f "$vc" ] || { echo "vcvars64.bat not found at $vc (set VCVARS64)" >&2; exit 1; }
  mkdir -p bin
  printf '@call "%s" >nul\r\n@set\r\n' "$(cygpath -w "$vc")" > bin/vcvars-env.bat
  local key value
  while IFS='=' read -r key value; do
    case "$key" in
      INCLUDE | LIB | LIBPATH) export "$key=$value" ;;
      Path | PATH) PATH="$(cygpath -u -p "$value")"; export PATH ;;
    esac
  done < <(cmd.exe //c "$(cygpath -w bin/vcvars-env.bat)" | tr -d '\r')
}

# Sets CC_KIND (msvc | gnu), CC_BIN, OBJ, OPT for compiler $1.
pick_compiler() {
  case "$1" in
    msvc)
      [ "$WIN" = 1 ] || { echo "c-msvc builds on Windows only" >&2; exit 1; }
      msvc_env
      CC_KIND=msvc; CC_BIN=cl.exe; OBJ=obj; OPT="-O2 -DNDEBUG" ;;
    clang | clang-o2)
      CC_KIND=gnu; OBJ=o; OPT="-O3 -DNDEBUG"
      if [ "$1" = clang-o2 ]; then OPT="-O2"; fi
      if [ -n "${CC_CLANG_HOME:-}" ]; then CC_BIN="$CC_CLANG_HOME/bin/clang$EXE"
      elif [ -n "${CPP_CLANG:-}" ]; then CC_BIN="$(dirname "$CPP_CLANG")/clang$EXE"
      else CC_BIN=clang; fi
      msvc_env ;;
    gcc)
      CC_KIND=gnu; OBJ=o; OPT="-O3 -DNDEBUG"
      if [ -n "${WINLIBS_HOME:-}" ]; then CC_BIN="$WINLIBS_HOME/bin/gcc$EXE"
      elif [ -n "${CPP_GCC:-}" ]; then CC_BIN="$(dirname "$CPP_GCC")/gcc$EXE"
      elif [ -n "${GCC:-}" ]; then CC_BIN="$GCC"
      elif command -v gcc-16 >/dev/null; then CC_BIN=gcc-16
      else CC_BIN=gcc; fi ;;
    *) echo "unknown compiler $1" >&2; exit 2 ;;
  esac
  command -v "$CC_BIN" >/dev/null 2>&1 || [ -x "$CC_BIN" ] || { echo "$CC_BIN not found" >&2; exit 1; }
}

is_x64() {
  if [ "$CC_KIND" = msvc ]; then return 0; fi
  case "$("$CC_BIN" -dumpmachine)" in x86_64* | amd64*) return 0 ;; *) return 1 ;; esac
}

# compile OUT_DIR "FLAGS" SOURCE...: one object per source into OUT_DIR, skipped when it already exists.
compile() {
  local out="$1" flags="$2"
  shift 2
  mkdir -p "$out"
  local src obj
  for src in "$@"; do
    obj="$out/$(basename "${src%.*}").$OBJ"
    [ -f "$obj" ] && continue
    if [ "$CC_KIND" = msvc ]; then
      # shellcheck disable=SC2086
      "$CC_BIN" -nologo $OPT $flags -c "$src" "-Fo$obj" >/dev/null
    else
      # shellcheck disable=SC2086
      "$CC_BIN" $OPT $flags -c "$src" -o "$obj"
    fi
  done
}

# Writes PCRE2's generic config.h, pcre2.h and character tables into $1, as its build does.
pcre2_generated() {
  mkdir -p "$1"
  cp "$PCRE2/src/config.h.generic" "$1/config.h"
  cp "$PCRE2/src/pcre2.h.generic" "$1/pcre2.h"
  cp "$PCRE2/src/pcre2_chartables.c.dist" "$1/pcre2_chartables.c"
}

PCRE2_FILES="auto_possess chkdint compile compile_cgroup compile_class config context convert dfa_match
  error extuni find_bracket jit_compile maketables match match_data match_next newline ord2utf
  pattern_info script_run serialize string_utils study substitute substring tables ucd valid_utf xclass"
# PCRE2's default build: 8-bit, Unicode on, JIT off.
PCRE2_DEFS="-DHAVE_CONFIG_H -DPCRE2_CODE_UNIT_WIDTH=8 -DPCRE2_STATIC -DSUPPORT_PCRE2_8 -DSUPPORT_UNICODE
  -DHAVE_STDINT_H -DHAVE_INTTYPES_H -DHAVE_LIMITS_H -DHAVE_STRING_H -DHAVE_STDLIB_H -DHAVE_ASSERT_H"

# zlib without its gzFile layer (gz*.c), which the kernels do not use.
ZLIB_FILES=""
for f in adler32 compress crc32 deflate infback inffast inflate inftrees trees uncompr zutil; do
  ZLIB_FILES="$ZLIB_FILES $ZLIB/$f.c"
done

# Our sources are C17; the libraries build with each compiler's default language mode, as their own
# build systems do.
our_std() {
  if [ "$CC_KIND" = msvc ]; then echo "-std:c17"; else echo "-std=c17"; fi
}

build_libs() {
  local d="$1" quiet="-w"
  local gen="$d/gen"
  pcre2_generated "$gen/pcre2"
  local pcre2_src="$gen/pcre2/pcre2_chartables.c" f
  for f in $PCRE2_FILES; do pcre2_src="$pcre2_src $PCRE2/src/pcre2_$f.c"; done
  local zstd_src
  zstd_src="$(ls "$ZSTD"/lib/common/*.c "$ZSTD"/lib/compress/*.c "$ZSTD"/lib/decompress/*.c)"
  local argon2_core=ref.c
  if is_x64; then argon2_core=opt.c; fi
  # shellcheck disable=SC2086
  {
    compile "$d/lib/zlib" "$quiet" $ZLIB_FILES &
    compile "$d/lib/zstd" "$quiet -DZSTD_DISABLE_ASM -I$ZSTD/lib -I$ZSTD/lib/common" $zstd_src &
    compile "$d/lib/pcre2" "$quiet $PCRE2_DEFS -I$gen/pcre2 -I$PCRE2/src" $pcre2_src &
    compile "$d/lib/mbedtls" "$quiet -I$MBEDTLS/include -I$MBEDTLS/library" \
      "$MBEDTLS/library/sha256.c" "$MBEDTLS/library/platform_util.c" &
    compile "$d/lib/cjson" "$quiet" "$CJSON/cJSON.c" &
    compile "$d/lib/argon2" "$quiet -I$ARGON2/include -I$ARGON2/src" "$ARGON2/src/argon2.c" \
      "$ARGON2/src/core.c" "$ARGON2/src/encoding.c" "$ARGON2/src/thread.c" \
      "$ARGON2/src/blake2/blake2b.c" "$ARGON2/src/$argon2_core" &
    compile "$d/lib/blowfish" "$quiet" "$BLOWFISH/crypt_blowfish.c" "$BLOWFISH/crypt_gensalt.c" &
    compile "$d/lib/libcsv" "$quiet" "$LIBCSV/libcsv.c" &
    compile "$d/lib/stb" "$quiet -I$STB -I$STB_RESIZE" vendor/stb_impl.c &
    wait
  }
}

our_includes() {
  local d="$1"
  echo "-I$d -I$ZLIB -I$ZSTD/lib -I$d/gen/pcre2 -I$MBEDTLS/include -I$CJSON -I$ARGON2/include -I$BLOWFISH" \
    "-I$LIBCSV -I$STB -I$STB_RESIZE -DPCRE2_STATIC"
}

our_warnings() {
  local werror="$1"
  if [ "$CC_KIND" = msvc ]; then
    echo "-W4 ${werror:+-WX}"
  else
    echo "-Wall -Wextra ${werror:+-Werror}"
  fi
}

windows_defs() {
  if [ "$WIN" = 1 ]; then echo "-D_CRT_SECURE_NO_WARNINGS"; fi
}

write_build_info() {
  local d="$1" cc="$2" flags
  if [ "$CC_KIND" = msvc ]; then flags="cl -std:c17 $OPT"; else flags="${cc%-o2} -std=c17 $OPT"; fi
  {
    echo "#define KERNELS_BUILD \"$flags\""
    for lib in zlib zstd pcre2 mbedtls cjson argon2 crypt_blowfish libcsv stb stb_resize; do
      echo "#define LIB_$(echo "$lib" | tr '[:lower:]' '[:upper:]') \"$lib $(lock_version "$lib")\""
    done
  } > "$d/build_info.h"
}

build() {
  local cc="$1"
  pick_compiler "$cc"
  [ -d "$ZLIB" ] || ./fetch_deps.sh
  local d="bin/$cc"
  mkdir -p "$d"
  write_build_info "$d" "$cc"
  build_libs "$d"
  rm -rf "$d/src"
  # shellcheck disable=SC2046,SC2086
  compile "$d/src" "$(our_std) $(our_warnings "") $(windows_defs) $(our_includes "$d")" src/*.c
  local objs
  objs="$(ls "$d"/src/*."$OBJ" "$d"/lib/*/*."$OBJ")"
  if [ "$CC_KIND" = msvc ]; then
    # shellcheck disable=SC2086
    "$CC_BIN" -nologo $objs "-Fe$d/kernels$EXE" bcrypt.lib >/dev/null
  elif [ "$WIN" = 1 ]; then
    # shellcheck disable=SC2086
    "$CC_BIN" $objs -o "$d/kernels$EXE" -lbcrypt
  else
    # shellcheck disable=SC2086
    "$CC_BIN" $objs -o "$d/kernels$EXE" -lm -lpthread
  fi
  echo "built $d/kernels$EXE ($("$d/kernels$EXE" fib --size 1 | sed 's/.*"runtime":"\([^"]*\)".*/\1/'))"
}

lint_compiler() {
  local cc="$1"
  pick_compiler "$cc"
  local d="bin/lint-$cc"
  rm -rf "$d"
  mkdir -p "$d"
  [ -d "$ZLIB" ] || ./fetch_deps.sh
  pcre2_generated "$d/gen/pcre2"
  write_build_info "$d" "$cc"
  # shellcheck disable=SC2046,SC2086
  compile "$d/src" "$(our_std) $(our_warnings 1) $(windows_defs) $(our_includes "$d")" src/*.c
  echo "lint $cc ($CC_BIN): no warnings"
}

lint() {
  uvx --from "clang-format==$CLANG_FORMAT_VERSION" clang-format --dry-run --Werror src/*.c src/*.h
  echo "clang-format $CLANG_FORMAT_VERSION: clean"
  local cc found=0
  for cc in msvc clang gcc; do
    if [ "$cc" = msvc ] && [ "$WIN" != 1 ]; then continue; fi
    (lint_compiler "$cc") && found=$((found + 1)) || { echo "lint $cc failed" >&2; exit 1; }
  done
  [ "$found" -gt 0 ] || { echo "no C compiler found" >&2; exit 1; }
}

case "${1:-}" in
  msvc | clang | clang-o2 | gcc) build "$1" ;;
  lint) lint ;;
  *) echo "usage: $0 msvc|clang|clang-o2|gcc|lint" >&2; exit 2 ;;
esac
