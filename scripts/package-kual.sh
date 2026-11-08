#!/bin/sh
# Backward-compatible entry point. The install package never contains ROMs.
set -eu
cd "$(dirname "$0")/.."
exec ./scripts/package-rom-free.sh
