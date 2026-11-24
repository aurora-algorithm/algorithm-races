#!/usr/bin/env bash
# Compiles target/kernels.jar (run `mvn -q -B package` first) ahead of time with GraalVM native-image
# into target/kernels-native (.exe on Windows). Stock options: no PGO, no -march, default (Serial) GC.
# Reflection metadata for the json kernel's records: src/main/resources/META-INF/native-image/.
set -euo pipefail
cd "$(dirname "$0")"

: "${GRAALVM_HOME:?set GRAALVM_HOME to a GraalVM JDK 25}"
case "$(uname -s)" in
  MINGW* | MSYS* | CYGWIN*) native_image="$GRAALVM_HOME/bin/native-image.cmd" ;;
  *) native_image="$GRAALVM_HOME/bin/native-image" ;;
esac

if [[ ! -f target/kernels.jar ]]; then
  echo "target/kernels.jar is missing: run mvn -q -B package first" >&2
  exit 1
fi

# native-image appends .exe itself on Windows.
"$native_image" -jar target/kernels.jar -o target/kernels-native
