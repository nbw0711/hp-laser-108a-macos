#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
[[ $(uname -s) == Darwin ]] || { echo 'macOS required' >&2; exit 1; }
mkdir -p build/obj
# ARCHS='arm64 x86_64' builds a Universal 2 driver. Defaults to this Mac.
read -r -a archs <<< "${ARCHS:-$(uname -m)}"
flags=(-mmacosx-version-min=12.0)
for arch in "${archs[@]}"; do
  case "$arch" in arm64|x86_64) flags+=(-arch "$arch");; *) echo 'Unsupported architecture' >&2; exit 1;; esac
done
for f in vendor/splix/src/*.cpp; do
  [[ "$f" == *pstoqpdl.cpp ]] && continue
  clang++ "${flags[@]}" -std=gnu++11 -O2 -Wno-deprecated-declarations \
    -DTHREADS=2 -DCACHESIZE=30 -DDISABLE_JBIG -Ivendor/splix/include \
    -c "$f" -o "build/obj/$(basename "$f" .cpp).o"
done
clang++ "${flags[@]}" build/obj/*.o -lcups -lpthread -o build/rastertoqpdl
clang "${flags[@]}" -O2 -Wall -Wextra -Wno-deprecated-declarations \
  src/hp108a-usb.c -framework IOKit -framework CoreFoundation -o build/hp108a
clang "${flags[@]}" -O2 -Wno-deprecated-declarations tests/mkraster.c -lcups -o build/mkraster
# Filter ownership/path are checked after installation; source-tree binaries are user-owned.
/usr/bin/cupstestppd -I filters -v ppd/hp108a.ppd
file build/rastertoqpdl build/hp108a
