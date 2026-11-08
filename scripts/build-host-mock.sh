#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
cmake -S . -B build-host-mock -DGOLDEN_MOCK_CORE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host-mock --parallel 2
./build-host-mock/golden_hero --help
