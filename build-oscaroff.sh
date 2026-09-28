#!/usr/bin/env bash
set -euo pipefail

cd /opt/PongoOS

LTO_LIB=/usr/lib/llvm-10/lib/libLTO.so

echo "== Toolchain =="
printf 'clang:          '; command -v clang
printf 'ld64:           '; command -v ld64
printf 'cctools-strip:  '; command -v cctools-strip
clang --version | head -n 1

echo
echo "== Newlib sysroot =="
test -f newlib/aarch64-none-darwin/include/sys/cdefs.h
echo "OK: newlib/aarch64-none-darwin/include/sys/cdefs.h"

echo
echo "== LLVM LTO =="
test -f "$LTO_LIB"
echo "OK: $LTO_LIB"
file "$LTO_LIB"

echo
echo "== Building oscaroff Pongo module =="

make -C example/oscaroff clean

# The copied testmodule Makefile removes build/ during `clean` but does not
# recreate it before invoking clang/ld64. ld64 also creates temporary files
# next to the requested output (build/oscaroff.ld_XXXXXX), so the directory
# must exist before the link starts.
mkdir -p example/oscaroff/build

make -C example/oscaroff \
    EMBEDDED_LDFLAGS="-fuse-ld=/usr/bin/ld64 -Wl,-lto_library,$LTO_LIB"

OUT=example/oscaroff/build/oscaroff

echo
echo "== Result =="
test -f "$OUT"
file "$OUT"
ls -lh "$OUT"

mkdir -p /out
cp -f "$OUT" /out/oscaroff
test -s /out/oscaroff

echo
echo "SUCCESS: /out/oscaroff"
