#!/bin/bash
# Fetches the pinned third-party sources of the C++ kernels into third_party/ (gitignored): each file is downloaded
# once into third_party/dl, checked against the sha256 below (a mismatch stops the script) and unpacked. Idempotent:
# a library whose folder exists is skipped. Needs curl, tar and sha256sum (Git Bash has them) or shasum (macOS).
#
# usage (from kernels/cpp):  ./deps.sh
set -euo pipefail
cd "$(dirname "$0")"
TP=third_party
DL="$TP/dl"
BOOST="$TP/boost-1.92.0"
mkdir -p "$DL" "$TP/include"

# name | url | sha256 | how to unpack (tgz, txz, boost, or a path under third_party/include for a single header)
# boost: one Boost module at the boost-1.92.0 tag; its include/ is merged into third_party/boost-1.92.0/include.
# Boost.Sort and Boost.Unordered (builtins group) are header-only; the other modules are the headers they include.
DEPS="
zlib-1.4.tar.gz|https://github.com/madler/zlib/releases/download/v1.4/zlib-1.4.tar.gz|02dd159783677a749c04adbdd02b55b8a2bcc7e054f51b76b549199a916ad584|tgz
zstd-1.5.7.tar.gz|https://github.com/facebook/zstd/releases/download/v1.5.7/zstd-1.5.7.tar.gz|eb33e51f49a15e023950cd7825ca74a4a2b43db8354825ac24fc1b7ee09e6fa3|tgz
Botan-3.14.1.tar.xz|https://botan.randombit.net/releases/Botan-3.14.1.tar.xz|5b85cba0f014d344eb8e9919aee0a3bdc31e16acb05816410025be9bee1e0930|txz
multiprecision-Boost_1_92_0.tar.gz|https://codeload.github.com/boostorg/multiprecision/tar.gz/refs/tags/Boost_1_92_0|d53a907779b1ba88c80c15953c24106bf92b1b4731678441a8655e527c9d3df1|tgz
json-3.12.0.hpp|https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp|aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63|nlohmann/json.hpp
ctre-3.11.0.hpp|https://raw.githubusercontent.com/hanickadot/compile-time-regular-expressions/v3.11.0/single-header/ctre.hpp|f0d9a70c782914f83066554cc0690e6c68c83f5bfbc8fab111d3f6affdcc653a|ctre.hpp
csv-parser-5.8.0.hpp|https://github.com/vincentlaucsb/csv-parser/releases/download/v5.8.0/csv.hpp|fe1ba29657e6db9968889738cb73c6643a0eda5992f54e9a228d86f5344b79e7|csv-parser/csv.hpp
fast-cpp-csv-parser-574a9fe.h|https://raw.githubusercontent.com/ben-strasser/fast-cpp-csv-parser/574a9fe4d323ba63416877a4a5fe59088d37aa34/csv.h|b1e79011724dcf504611f5a29dc6927fccf79bb6049b17fed339b10a30e0fe75|fast-cpp-csv-parser/csv.h
stb_image-9600522.h|https://raw.githubusercontent.com/nothings/stb/9600522de53973336fa033a289858a4354cbb04c/stb_image.h|deb4f18c9ca21b751a1323019031c94c0bf5ca1d2929ef5913cc56f00aab6e91|stb/stb_image.h
stb_image_write-9600522.h|https://raw.githubusercontent.com/nothings/stb/9600522de53973336fa033a289858a4354cbb04c/stb_image_write.h|a8e7f77f0ccfdc9bac09a71268b131931ff81aafca82c393aeb6aaba10d9e7ca|stb/stb_image_write.h
stb_image_resize2-703100c.h|https://raw.githubusercontent.com/nothings/stb_image_resize/703100c5f8a0b75b4861e401db567f1d82d545b8/stb_image_resize2.h|7420ce124ccd746e47f66d85235eb22838b339ae0689a72d6eb2e3d32dc657d1|stb/stb_image_resize2.h
pdqsort-b1ef26a.h|https://raw.githubusercontent.com/orlp/pdqsort/b1ef26a55cdb60d236a5cb199c4234c704f46726/pdqsort.h|1916aff237fc7ecc638dec6397662334552a1b6f823fa65011a8dfe831237fc0|pdqsort/pdqsort.h
unordered_dense-5.3.3.h|https://raw.githubusercontent.com/martinus/unordered_dense/v5.3.3/include/ankerl/unordered_dense.h|29f36d9a3b481c2ec07d9382fc6ffc0301f84dcca8395db63c0ce83922ad9aa2|ankerl/unordered_dense.h
unordered_dense-5.3.3-stl.h|https://raw.githubusercontent.com/martinus/unordered_dense/v5.3.3/include/ankerl/stl.h|6ae137a72fd9f6c197e92103500090b3eea9057db046c9defcc41840b855f134|ankerl/stl.h
boost-assert-1.92.0.tar.gz|https://codeload.github.com/boostorg/assert/tar.gz/refs/tags/boost-1.92.0|8d49fc69df12e8fbc0e7fe3c4ede45044747199ccdeaf66c2aebb146becc54aa|boost
boost-config-1.92.0.tar.gz|https://codeload.github.com/boostorg/config/tar.gz/refs/tags/boost-1.92.0|b4171037f13373203ba79cbc141d612982052283e696a315185ab5bea46102a0|boost
boost-container_hash-1.92.0.tar.gz|https://codeload.github.com/boostorg/container_hash/tar.gz/refs/tags/boost-1.92.0|f88aabbbc2f163db5eea65662b26e5af2bde0a0bff065c645b7347ed6a71a566|boost
boost-core-1.92.0.tar.gz|https://codeload.github.com/boostorg/core/tar.gz/refs/tags/boost-1.92.0|aa4eb2ccbe5577ded70f25b8f0d852a5246591f90cc34916160739e3d0dbfea6|boost
boost-describe-1.92.0.tar.gz|https://codeload.github.com/boostorg/describe/tar.gz/refs/tags/boost-1.92.0|4ef61ea2ca967b803e171f4756e03e201680d5dd19d014324f9a848e0d06863b|boost
boost-iterator-1.92.0.tar.gz|https://codeload.github.com/boostorg/iterator/tar.gz/refs/tags/boost-1.92.0|5f07301876b74af4387b4e7d8f3d01172978c9c321ca403791307de24a5b775d|boost
boost-mp11-1.92.0.tar.gz|https://codeload.github.com/boostorg/mp11/tar.gz/refs/tags/boost-1.92.0|a5754dc5ff9e7ff34119983889530e3ab8d8fb43ce2b4fea9c18673044df45d9|boost
boost-mpl-1.92.0.tar.gz|https://codeload.github.com/boostorg/mpl/tar.gz/refs/tags/boost-1.92.0|4b59f9b860064c74431dcdb7c64b3425f570dea65ea9742a2c1ec1ebbe9fb1fd|boost
boost-predef-1.92.0.tar.gz|https://codeload.github.com/boostorg/predef/tar.gz/refs/tags/boost-1.92.0|4c870bcf7f547f3e3d1db26700b078a33f87c4c8232cd1c8d89ec82be6b54130|boost
boost-preprocessor-1.92.0.tar.gz|https://codeload.github.com/boostorg/preprocessor/tar.gz/refs/tags/boost-1.92.0|7c8f444bb5cf5f37d594ca421938ef0271f7d9866275347f25ceabffb931de7d|boost
boost-range-1.92.0.tar.gz|https://codeload.github.com/boostorg/range/tar.gz/refs/tags/boost-1.92.0|b8f11b8d35a18c5ea0d822102e5ae977d1a84e3842a8fe11c397425488415ec2|boost
boost-sort-1.92.0.tar.gz|https://codeload.github.com/boostorg/sort/tar.gz/refs/tags/boost-1.92.0|b25005996555c9ed7b6c2bb0cdd42bd0fbed4b73ae59a6bf2c4527d7b89f6578|boost
boost-static_assert-1.92.0.tar.gz|https://codeload.github.com/boostorg/static_assert/tar.gz/refs/tags/boost-1.92.0|c528438060c45a4b17ca84ebdce0f3cbcc1cd275f3ad2752b1b0d0ff0b220805|boost
boost-throw_exception-1.92.0.tar.gz|https://codeload.github.com/boostorg/throw_exception/tar.gz/refs/tags/boost-1.92.0|1ddbb967a43504e28bbe2a35a010df9f7ff5d1a49d3678c9fdfc035fd32ce005|boost
boost-type_traits-1.92.0.tar.gz|https://codeload.github.com/boostorg/type_traits/tar.gz/refs/tags/boost-1.92.0|ee6c4d3d993f9138d9a77c57c3d91111aafed79add4f09f4e0a7effcfcdc0cae|boost
boost-unordered-1.92.0.tar.gz|https://codeload.github.com/boostorg/unordered/tar.gz/refs/tags/boost-1.92.0|cc2eac1c597c221f20d166273825bba3581e64b75c295e97eb67bd5444d4bef0|boost
"

sha256() {
  if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1" | cut -c1-64; else shasum -a 256 "$1" | cut -c1-64; fi
}

fetch() {
  local name="$1" url="$2" want="$3" got
  if [ ! -s "$DL/$name" ]; then
    echo "  download $url"
    curl -fsSL --retry 3 -o "$DL/$name.part" "$url"
    mv "$DL/$name.part" "$DL/$name"
  fi
  got=$(sha256 "$DL/$name")
  if [ "$got" != "$want" ]; then
    echo "error: $name: sha256 $got, pinned $want (delete $DL/$name to download again)" >&2
    exit 1
  fi
}

unpack() {
  local name="$1" how="$2" dest tmp
  case "$how" in
    tgz | txz)
      dest="$TP/${name%.tar.*}"
      [ -d "$dest" ] && return
      tmp="$TP/.unpack-$$"
      rm -rf "$tmp" && mkdir -p "$tmp"
      tar -xf "$DL/$name" -C "$tmp" --exclude="*/tests/*" --exclude="*/test/*"
      mv "$tmp"/* "$dest"
      rm -rf "$tmp" ;;
    boost)
      dest="$BOOST/.${name%.tar.*}"
      [ -f "$dest" ] && return
      tmp="$TP/.unpack-$$"
      rm -rf "$tmp" && mkdir -p "$tmp" "$BOOST/include"
      tar -xf "$DL/$name" -C "$tmp" --exclude="*/tests/*" --exclude="*/test/*"
      cp -R "$tmp"/*/include/. "$BOOST/include/"
      rm -rf "$tmp"
      touch "$dest" ;;
    *)
      dest="$TP/include/$how"
      [ -f "$dest" ] && return
      mkdir -p "$(dirname "$dest")"
      cp "$DL/$name" "$dest" ;;
  esac
}

while IFS='|' read -r name url sha how; do
  [ -n "$name" ] || continue
  case "$how" in
    tgz | txz) [ -d "$TP/${name%.tar.*}" ] && continue ;;
    boost) [ -f "$BOOST/.${name%.tar.*}" ] && continue ;;
    *) [ -f "$TP/include/$how" ] && continue ;;
  esac
  fetch "$name" "$url" "$sha"
  unpack "$name" "$how"
  echo "  $name: ok"
done <<<"$DEPS"
