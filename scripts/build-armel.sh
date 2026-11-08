#!/bin/sh
# Cross-compile for Kindle 4 / Touch / Paperwhite 1 (kindle5 ABI).
set -eu
cd "$(dirname "$0")/.."
PREFIX="${KINDLE5_PREFIX:-${HOME}/x-tools/arm-kindle5-linux-gnueabi/bin/arm-kindle5-linux-gnueabi}"
CXX="${CXX:-${PREFIX}-g++}"

if [ ! -x "$CXX" ] && ! command -v "$CXX" >/dev/null 2>&1; then
  echo "KPW1 compiler not found: $CXX" >&2
  echo "Run scripts/bootstrap-kindle5-toolchain.sh first, or set KINDLE5_PREFIX." >&2
  exit 2
fi
if [ ! -f upstream/nc1020.cpp ]; then
  echo "Fetching pinned, source-only NC1020 emulator code..."
  python3 scripts/fetch_upstream.py
fi

cmake -S . -B build-armel \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=arm \
  -DCMAKE_CXX_COMPILER="$CXX" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG -mfloat-abi=soft -static-libstdc++ -static-libgcc"
cmake --build build-armel --parallel 2

BIN=build-armel/golden_hero
command -v readelf >/dev/null 2>&1 || { echo "readelf is required for ABI verification" >&2; exit 3; }
readelf -h "$BIN" | grep -q 'Machine:.*ARM' || { echo "Not an ARM ELF" >&2; exit 4; }
if readelf -A "$BIN" | grep -q 'Tag_ABI_VFP_args: VFP registers'; then
  echo "Hard-float ABI detected; refusing KPW1 build." >&2; exit 5
fi
readelf -l "$BIN" | grep -q '/lib/ld-linux.so.3' || {
  echo "Unexpected ELF interpreter; KPW1 expects /lib/ld-linux.so.3." >&2; exit 6;
}
echo "KPW1 ARM soft-float build created: $BIN"
file "$BIN"
readelf -h "$BIN" | grep -E 'Class:|Data:|Machine:|Flags:' || true
readelf -A "$BIN" | grep -E 'Tag_CPU_arch|Tag_ABI_VFP_args' || true
readelf -l "$BIN" | grep 'Requesting program interpreter' || true
readelf -d "$BIN" | grep NEEDED || true
